#include "fast_scan.h"

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG         NextEntryOffset;
    ULONG         FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG         FileAttributes;
    ULONG         FileNameLength;
    ULONG         EaSize;
    CCHAR         ShortNameLength;
    WCHAR         ShortName[12];
    WCHAR         FileName[1];
} FILE_BOTH_DIR_INFORMATION, * PFILE_BOTH_DIR_INFORMATION;

typedef NTSTATUS(__stdcall* pNtQueryDirectoryFileEx)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, ULONG, PUNICODE_STRING);
typedef NTSTATUS(__stdcall* pNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
typedef void(__stdcall* pRtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
typedef NTSTATUS(__stdcall* pNtClose)(HANDLE);

pNtQueryDirectoryFileEx g_NtQueryDirectoryFileEx = NULL;
pNtOpenFile g_NtOpenFile = NULL;
pRtlInitUnicodeString g_RtlInitUnicodeString = NULL;
pNtClose g_NtClose = NULL;

typedef struct _DirNode {
    SLIST_ENTRY ItemEntry;
    WCHAR path[MAX_PATH];
} DirNode;

typedef struct _NodeBlock {
    struct _NodeBlock* next;
    DirNode nodes[NODE_BLOCK_SIZE];
} NodeBlock;

SLIST_HEADER g_nodeFreeList;
volatile NodeBlock* g_nodeBlockHead = NULL;

volatile DirNode* g_dirHead = NULL;
volatile LONG g_dirTasks = 0;

typedef struct {
    FOUND_JAR_CALLBACK callback;
    void* context;
} ThreadScanArgs;


// this memory pool tries to avoid heap fragmentation when the user scans multiple times with the same scan mode, or alternating between them
static void DestroyNodePool() {
    NodeBlock* current = (NodeBlock*)g_nodeBlockHead;
    while (current) {
        NodeBlock* next = current->next;
        HeapFree(GetProcessHeap(), 0, current);
        current = next;
    }
    g_nodeBlockHead = NULL;

    InitializeSListHead(&g_nodeFreeList);
}

static DirNode* AllocateNode() {
    DirNode* node = (DirNode*)InterlockedPopEntrySList(&g_nodeFreeList);
    if (node == NULL) {
        NodeBlock* newBlock = (NodeBlock*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NodeBlock));
        if (newBlock == NULL) return NULL;

        NodeBlock* oldHead;
        do {
            oldHead = (NodeBlock*)_InterlockedCompareExchangePointer((PVOID volatile*)&g_nodeBlockHead, NULL, NULL);
            newBlock->next = oldHead;
        } while (InterlockedCompareExchangePointer((PVOID volatile*)&g_nodeBlockHead, newBlock, oldHead) != oldHead);

        for (int i = 0; i < NODE_BLOCK_SIZE; ++i) {
            InterlockedPushEntrySList(&g_nodeFreeList, (PSLIST_ENTRY)&newBlock->nodes[i]);
        }
        node = (DirNode*)InterlockedPopEntrySList(&g_nodeFreeList);
    }
    return node;
}

static void __fastcall FreeNode(DirNode* node) {
    if (node) InterlockedPushEntrySList(&g_nodeFreeList, (PSLIST_ENTRY)node);
}


static void __fastcall PushDir(const WCHAR* p) {
    DirNode* node = AllocateNode();
    if (!node) return;
    if (FAILED(StringCchCopyW(node->path, MAX_PATH, p))) {
        FreeNode(node);
        return;
    }

    DirNode* oldHead;
    do {
        oldHead = (DirNode*)InterlockedCompareExchangePointer((PVOID volatile*)&g_dirHead, NULL, NULL);
        // this is not a real linked list, just a way to use the SLIST entry for linking in the queue
        // we will cast back to DirNode when popping; the first member is the SLIST_ENTRY
        ((PSLIST_ENTRY)node)->Next = (PSLIST_ENTRY)oldHead;
    } while (InterlockedCompareExchangePointer((PVOID volatile*)&g_dirHead, node, oldHead) != oldHead);
}

static DirNode* PopDir() {
    DirNode* oldHead;
    do {
        oldHead = (DirNode*)InterlockedCompareExchangePointer((PVOID volatile*)&g_dirHead, NULL, NULL);
        if (!oldHead) {
            return NULL;
        }
    } while (InterlockedCompareExchangePointer((PVOID volatile*)&g_dirHead, (void*)((PSLIST_ENTRY)oldHead)->Next, oldHead) != oldHead);
    return oldHead;
}


static bool __fastcall IsJarFile(const WCHAR* name) {
    size_t len;
    if (FAILED(StringCchLengthW(name, MAX_PATH, &len))) return FALSE;
    return (len >= 4 && _wcsicmp(name + len - 4, L".jar") == 0);
}

// https://blog.s-schoener.com/2024-06-24-find-files-internals/
static void __fastcall ProcessDirectoryNt(const WCHAR* dirPath, FOUND_JAR_CALLBACK callback, void* context) {
    HANDLE hDir = NULL;
    OBJECT_ATTRIBUTES objAttr = { 0 };
    UNICODE_STRING uPath;
    IO_STATUS_BLOCK ioStatus;

    WCHAR ntPath[MAX_PATH];
    if (FAILED(StringCchCopyW(ntPath, MAX_PATH, L"\\??\\"))) return;
    if (FAILED(StringCchCatW(ntPath, MAX_PATH, dirPath))) return;

    g_RtlInitUnicodeString(&uPath, ntPath);
    InitializeObjectAttributes(&objAttr, &uPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

    const NTSTATUS status_open = g_NtOpenFile(&hDir, FILE_LIST_DIRECTORY | SYNCHRONIZE, &objAttr, &ioStatus,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

    if (!NT_SUCCESS(status_open) || hDir == NULL) return;

    BYTE* buffer = (BYTE*)_aligned_malloc(DIR_BUFFER_SIZE, 16);
    if (!buffer) {
        g_NtClose(hDir);
        return;
    }

    bool bRestartScan = TRUE;
    NTSTATUS status_query;
    while (TRUE) {
        status_query = g_NtQueryDirectoryFileEx(hDir, NULL, NULL, NULL, &ioStatus, buffer, DIR_BUFFER_SIZE, FileBothDirectoryInformation, bRestartScan ? SL_RESTART_SCAN : 0, NULL);
        if (!NT_SUCCESS(status_query) || status_query == STATUS_NO_MORE_FILES) break;

        bRestartScan = FALSE;
        PFILE_BOTH_DIR_INFORMATION pInfo = (PFILE_BOTH_DIR_INFORMATION)buffer;
        while (TRUE) {
            const size_t nameLen = pInfo->FileNameLength / sizeof(WCHAR);
            if (nameLen > 0 && nameLen < MAX_PATH) {
                WCHAR fileName[MAX_PATH];
                if (SUCCEEDED(StringCchCopyNW(fileName, MAX_PATH, pInfo->FileName, nameLen))) {
                    fileName[nameLen] = L'\0';

                    if (wcscmp(fileName, L".") != 0 && wcscmp(fileName, L"..") != 0 &&
                        !(pInfo->FileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_OFFLINE))) {

                        WCHAR fullPath[MAX_PATH];
                        if (SUCCEEDED(PathCchCombine(fullPath, MAX_PATH, dirPath, fileName))) {
                            if (pInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                                if (_wcsicmp(fileName, L"System Volume Information") != 0 &&
                                    _wcsicmp(fileName, L"$RECYCLE.BIN") != 0 &&
                                    _wcsicmp(fileName, L"Recovery") != 0) {
                                    InterlockedIncrement(&g_dirTasks);
                                    PushDir(fullPath);
                                }
                            }
                            else if (IsJarFile(fileName)) {
                                if (callback) {
                                    callback(fullPath, context);
                                }
                            }
                        }
                    }
                }
            }
            if (pInfo->NextEntryOffset == 0) break;
            pInfo = (PFILE_BOTH_DIR_INFORMATION)((BYTE*)pInfo + pInfo->NextEntryOffset);
        }
    }
    _aligned_free(buffer);
    g_NtClose(hDir);
}

DWORD WINAPI ThreadScan(LPVOID lpParam) {
    ThreadScanArgs* args = (ThreadScanArgs*)lpParam;
    if (!args) {
        InterlockedDecrement(&g_dirTasks);
        return 1;
    }

    while (TRUE) {
        DirNode* dn = PopDir();
        if (!dn) {
            if (InterlockedCompareExchange(&g_dirTasks, 0, 0) == 0) break;
            _mm_pause(); // hint to cpu we're in a spin-wait
            continue;
        }
        ProcessDirectoryNt(dn->path, args->callback, args->context);
        FreeNode(dn);
        InterlockedDecrement(&g_dirTasks);
    }
    return 0;
}

typedef struct {
    WCHAR letter;
    bool isSSD;
} DriveInfo;

static bool IsSSD(const WCHAR driveLetter) {
    WCHAR devicePath[] = L"\\\\.\\?:"; devicePath[4] = driveLetter;
    const HANDLE hDevice = CreateFileW(devicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) return FALSE;

    STORAGE_PROPERTY_QUERY query = { .PropertyId = StorageDeviceSeekPenaltyProperty, .QueryType = PropertyStandardQuery };
    DEVICE_SEEK_PENALTY_DESCRIPTOR dspd = { 0 };
    DWORD dwBytesReturned = 0;
    bool result = FALSE;
    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &dspd, sizeof(dspd), &dwBytesReturned, NULL)) {
        result = (dspd.IncursSeekPenalty == FALSE);
    }
    CloseHandle(hDevice);
    return result;
}

static int CompareDrives(const void* a, const void* b) {
    const DriveInfo* driveA = (const DriveInfo*)a;
    const DriveInfo* driveB = (const DriveInfo*)b;
    if (driveA->isSSD && !driveB->isSSD) return -1;
    if (!driveA->isSSD && driveB->isSSD) return 1;
    return 0;
}

static bool InitializeNtApi() {
    const HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return FALSE;
    g_NtQueryDirectoryFileEx = (pNtQueryDirectoryFileEx)GetProcAddress(hNtdll, "NtQueryDirectoryFileEx");
    g_NtOpenFile = (pNtOpenFile)GetProcAddress(hNtdll, "NtOpenFile");
    g_RtlInitUnicodeString = (pRtlInitUnicodeString)GetProcAddress(hNtdll, "RtlInitUnicodeString");
    g_NtClose = (pNtClose)GetProcAddress(hNtdll, "NtClose");
    return g_NtQueryDirectoryFileEx && g_NtOpenFile && g_RtlInitUnicodeString && g_NtClose;
}

void RunUnstableScan(FOUND_JAR_CALLBACK callback, void* context) {
    if (!InitializeNtApi()) {
        return;
    }

    DriveInfo drives[26] = { 0 };
    int driveCount = 0;
    const DWORD driveMask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (_bittest((const LONG*)&driveMask, i)) {
            const WCHAR driveLetter = (WCHAR)('A' + i);
            const WCHAR rootPath[] = { driveLetter, L':', L'\\', L'\0' };
            if (GetDriveTypeW(rootPath) == DRIVE_FIXED) {
                drives[driveCount].letter = driveLetter;
                drives[driveCount].isSSD = IsSSD(driveLetter);
                driveCount++;
            }
        }
    }
    qsort(drives, driveCount, sizeof(DriveInfo), CompareDrives);

    g_dirHead = NULL;
    InterlockedExchange(&g_dirTasks, 0);
    for (int i = 0; i < driveCount; ++i) {
        const WCHAR rootPath[] = { drives[i].letter, L':', L'\\', L'\0' };
        InterlockedIncrement(&g_dirTasks);
        PushDir(rootPath);
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    const DWORD coreCount = sysInfo.dwNumberOfProcessors > 0 ? sysInfo.dwNumberOfProcessors : 1;
    HANDLE* threads = (HANDLE*)HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE) * coreCount);
    if (!threads) {
        // any remaining directories in the queue
        DirNode* dn;
        while ((dn = PopDir()) != NULL) {
            FreeNode(dn);
        }
        DestroyNodePool();
        return;
    }

    ThreadScanArgs threadArgs = { callback, context };

    for (DWORD i = 0; i < coreCount; ++i) {
        threads[i] = CreateThread(NULL, 0, ThreadScan, &threadArgs, 0, NULL);
        if (threads[i] == NULL) {
            threads[i] = NULL;
            continue;
        }
        SetThreadAffinityMask(threads[i], (DWORD_PTR)1 << i);
        SetThreadPriority(threads[i], THREAD_PRIORITY_TIME_CRITICAL);
    }

    WaitForMultipleObjects(coreCount, threads, TRUE, INFINITE);

    for (DWORD i = 0; i < coreCount; ++i) {
        if (threads[i]) CloseHandle(threads[i]);
    }
    HeapFree(GetProcessHeap(), 0, threads);

    // for any remaining directories in the queue
    DirNode* dn;
    while ((dn = PopDir()) != NULL) {
        FreeNode(dn);
    }

    DestroyNodePool();
}