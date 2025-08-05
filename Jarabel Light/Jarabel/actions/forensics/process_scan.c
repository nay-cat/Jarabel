#include "scans.h"

static void InitializeProcessScan() {
    EnterCriticalSection(&g_listLock);
    ClearMasterList(&g_processesMasterList);
    LeaveCriticalSection(&g_listLock);
    ListView_DeleteAllItems(hProcessList);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
}

static void AddProcessInfoToList(const WCHAR* __restrict tempPath, int* __restrict count) {
    FileInfo* pInfo = (FileInfo*)malloc(sizeof(FileInfo));
    if (pInfo) {
        wcscpy_s(pInfo->szFilePath, MAX_PATH, tempPath);
        WIN32_FILE_ATTRIBUTE_DATA fad = { 0 };
        if (GetFileAttributesExW(tempPath, GetFileExInfoStandard, &fad)) {
            pInfo->liFileSize.LowPart = fad.nFileSizeLow;
            pInfo->liFileSize.HighPart = fad.nFileSizeHigh;
            pInfo->ftLastAccessTime = fad.ftLastAccessTime;
        }
        else {
            pInfo->liFileSize.QuadPart = (ULONGLONG)-1;
        }
        pInfo->isObfuscated = FALSE;
        pInfo->entropy = 0;

        EnterCriticalSection(&g_listLock);
        AddToMasterList(&g_processesMasterList, pInfo);
        LeaveCriticalSection(&g_listLock);

        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = (*count)++;
        lvi.lParam = (LPARAM)pInfo;
        ListView_InsertItem(hProcessList, &lvi);
    }
}

static void FindJarPathsInMemory(BYTE* __restrict buffer, SIZE_T bytesRead, WCHAR*** __restrict p_foundPaths, int* __restrict foundCount, int* __restrict listCount, int* __restrict capacity, HWND hwnd) {
    const WCHAR* targetPattern = L"-jar";
    const size_t patternLen = 4; // wcslen(L"-jar")
    WCHAR** foundPaths = *p_foundPaths;

    size_t searchLimit = (bytesRead > (patternLen * sizeof(WCHAR))) ? (bytesRead - (patternLen * sizeof(WCHAR))) : 0;

    for (size_t j = 0; j <= searchLimit; j += sizeof(WCHAR)) {
        if (_wcsnicmp((WCHAR*)(buffer + j), targetPattern, patternLen) == 0) {
            WCHAR* match_ptr = (WCHAR*)(buffer + j + (patternLen * sizeof(WCHAR)));
            while (*match_ptr == L' ' || *match_ptr == L'\t') { match_ptr++; }
            if (*match_ptr == L'\"') {
                match_ptr++;
                WCHAR* end_quote = wcschr(match_ptr, L'\"');
                if (end_quote) {
                    size_t path_len = end_quote - match_ptr;
                    if (path_len > 0 && path_len < MAX_PATH) {
                        WCHAR tempPath[MAX_PATH];
                        memcpy(tempPath, match_ptr, path_len * sizeof(WCHAR));
                        tempPath[path_len] = L'\0';

                        BOOL isDuplicate = FALSE;
                        for (int k = 0; k < *foundCount; k++) {
                            if (wcscmp(foundPaths[k], tempPath) == 0) {
                                isDuplicate = TRUE;
                                break;
                            }
                        }
                        if (!isDuplicate) {
                            if (*foundCount >= *capacity) {
                                *capacity *= 2;
                                WCHAR** temp = realloc(foundPaths, *capacity * sizeof(WCHAR*));
                                if (temp == NULL) {
                                    MessageBoxW(hwnd, L"Memory reallocation failed. Results may be incomplete.", L"Warning", MB_ICONWARNING);
                                    return;
                                }
                                foundPaths = temp;
                                *p_foundPaths = temp;
                            }
                            foundPaths[(*foundCount)++] = _wcsdup(tempPath);
                            AddProcessInfoToList(tempPath, listCount);
                        }
                        j += (end_quote - (WCHAR*)(buffer + j)) * sizeof(WCHAR);
                    }
                }
            }
        }
    }
}

static void ScanWmiProcessMemory(HANDLE hProcess, WCHAR*** __restrict p_foundPaths, int* __restrict foundCount, int* __restrict listCount, int* __restrict capacity, HWND hwnd) {
    MEMORY_BASIC_INFORMATION mbi;
    unsigned char* p = NULL;

    while (VirtualQueryEx(hProcess, p, &mbi, sizeof(mbi)) == sizeof(mbi)) {
        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE && (mbi.Protect & (PAGE_READWRITE | PAGE_READONLY))) {
            BYTE* buffer = (BYTE*)malloc(mbi.RegionSize);
            if (buffer) {
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer, mbi.RegionSize, &bytesRead)) {
                    FindJarPathsInMemory(buffer, bytesRead, p_foundPaths, foundCount, listCount, capacity, hwnd);
                }
                free(buffer);
            }
        }
        p += mbi.RegionSize;
    }
}


static void FinalizeProcessScan(HWND hwnd, int count, WCHAR** __restrict foundPaths, int foundCount) {
    for (int k = 0; k < foundCount; k++) {
        free(foundPaths[k]);
    }
    free(foundPaths);

    if (count == 0) {
        MessageBoxW(hwnd, L"No executed JAR commands found in WMIPrvSE.exe memory. Run as Administrator if unsure.", L"Scan Finished", MB_ICONINFORMATION);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void HandleProcessScan(HWND hwnd) {
    InitializeProcessScan();

    const DWORD processArraySize = 4096;
    // to avoid overflow, otherwise function would use 16952 bytes of stack
    DWORD* aProcesses = malloc(sizeof(DWORD) * processArraySize);
    if (!aProcesses) {
        MessageBoxW(hwnd, L"Memory allocation failed for process list.", L"Error", MB_ICONERROR);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    DWORD cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(DWORD) * processArraySize, &cbNeeded)) {
        MessageBoxW(hwnd, L"Could not enumerate processes.", L"Error", MB_ICONERROR);
        free(aProcesses);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    cProcesses = cbNeeded / sizeof(DWORD);
    int count = 0;
    int capacity = 10;
    int foundCount = 0;
    WCHAR** foundPaths = malloc(capacity * sizeof(WCHAR*));
    if (!foundPaths) {
        MessageBoxW(hwnd, L"Memory allocation failed.", L"Error", MB_ICONERROR);
        free(aProcesses);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    for (unsigned int i = 0; i < cProcesses; i++) {
        if (aProcesses[i] == 0) continue;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
        if (hProcess == NULL) continue;

        WCHAR szProcessName[MAX_PATH];
        if (GetModuleBaseNameW(hProcess, NULL, szProcessName, MAX_PATH) && _wcsicmp(szProcessName, L"WMIPrvSE.exe") == 0) {
            ScanWmiProcessMemory(hProcess, &foundPaths, &foundCount, &count, &capacity, hwnd);
        }
        CloseHandle(hProcess);
    }

    FinalizeProcessScan(hwnd, count, foundPaths, foundCount);

    free(aProcesses);
}