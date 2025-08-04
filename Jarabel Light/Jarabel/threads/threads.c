#pragma optimize("gt", on)         
#pragma inline_depth(64)           
#pragma inline_recursion(on)      
#pragma auto_inline(on)

#include "../core/app.h"
#include "threads.h"

static __forceinline void ClearAllJarLists() {
    ClearMasterList(&g_allJarsMasterList);
    g_mavenMasterList.count = 0;
    g_gradleMasterList.count = 0;
    g_forgeMasterList.count = 0;
    g_fabricMasterList.count = 0;
    g_mcpMasterList.count = 0;
    g_libsMasterList.count = 0;
    g_runnableJarsMasterList.count = 0;

    ListView_DeleteAllItems(hMavenList); ListView_DeleteAllItems(hGradleList);
    ListView_DeleteAllItems(hForgeList); ListView_DeleteAllItems(hFabricList);
    ListView_DeleteAllItems(hMcpList); ListView_DeleteAllItems(hLibsList);
    ListView_DeleteAllItems(hRunnableJarsList);
}

static void __vectorcall ProcessDirectory(
    const WCHAR* __restrict dir,
    HWND hOwnerWnd,
    ScanMode mode,
    WorkQueue* __restrict queue,
    ThreadLocalJarLists* __restrict tlsLists
);

#pragma optimize("", on)

static void __vectorcall ProcessDirectory(const WCHAR* __restrict dir, HWND hOwnerWnd, ScanMode mode, WorkQueue* __restrict queue, ThreadLocalJarLists* __restrict tlsLists) {
    HANDLE hDir;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objAttr = { 0 };
    UNICODE_STRING uDirName;
    WCHAR ntPath[MAX_PATH];

    wcscpy_s(ntPath, MAX_PATH, L"\\??\\");
    wcscat_s(ntPath, MAX_PATH, dir);

    g_RtlInitUnicodeString(&uDirName, ntPath);
    InitializeObjectAttributes(&objAttr, &uDirName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = g_NtOpenFile(&hDir, FILE_LIST_DIRECTORY | SYNCHRONIZE, &objAttr, &ioStatusBlock, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
    if (!NT_SUCCESS(status) || hDir == INVALID_HANDLE_VALUE) {
        return;
    }

    ULONG bufferSize = 65536;
    BYTE* buffer = (BYTE*)_aligned_malloc(bufferSize, 16); // aligned so compiler can emit sse4.2 later if clang, sse2 if msvc
    if (!buffer) {
        g_NtClose(hDir);
        return;
    }

    while (TRUE) {
        status = g_NtQueryDirectoryFileEx(hDir, NULL, NULL, NULL, &ioStatusBlock, buffer, bufferSize, FileBothDirectoryInformation, 0, NULL);

        // ioStatusBlock.Information == 0 = checking for empty dirs
        if (!NT_SUCCESS(status) || status == STATUS_NO_MORE_FILES || ioStatusBlock.Information == 0) {
            break;
        }

        PFILE_BOTH_DIR_INFORMATION pDirInfo = (PFILE_BOTH_DIR_INFORMATION)buffer;
        while (TRUE) {
            WCHAR currentFileName[MAX_PATH];
            size_t nameLenInChars = pDirInfo->FileNameLength / sizeof(WCHAR);

            if (nameLenInChars > 0 && nameLenInChars < MAX_PATH) {
                memcpy(currentFileName, pDirInfo->FileName, pDirInfo->FileNameLength);
                currentFileName[nameLenInChars] = L'\0'; 

                if (wcscmp(currentFileName, L".") != 0 && wcscmp(currentFileName, L"..") != 0) {
                    WCHAR fullPath[MAX_PATH];
                    wcscpy_s(fullPath, MAX_PATH, dir);
                    PathAppendW(fullPath, currentFileName); 

                    if (pDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (wcscmp(currentFileName, L"$RECYCLE.BIN") != 0 && wcscmp(currentFileName, L"System Volume Information") != 0) {
                            if (mode == SCAN_MODE_RECURSIVE) {
                                ProcessDirectory(fullPath, hOwnerWnd, SCAN_MODE_RECURSIVE, NULL, NULL);
                            }
                            else {
                                Queue_Push(queue, fullPath);
                            }
                        }
                    }
                    else { // it's a file
                        const WCHAR* ext = PathFindExtensionW(currentFileName);
                        if (ext && _wcsicmp(ext, L".jar") == 0) {
                            FileInfo* pInfo = (FileInfo*)malloc(sizeof(FileInfo));
                            if (pInfo) {
                                wcscpy_s(pInfo->szFilePath, MAX_PATH, fullPath);
                                AnalyzeAndCategorizeJar(pInfo, hOwnerWnd, tlsLists);
                            }
                        }
                    }
                }
            }

            if (pDirInfo->NextEntryOffset == 0) break;
            pDirInfo = (PFILE_BOTH_DIR_INFORMATION)((BYTE*)pDirInfo + pDirInfo->NextEntryOffset);
        }
    }

    _aligned_free(buffer);
    g_NtClose(hDir);
}

unsigned __stdcall ScanWorkerThread(void* pArguments) {
    ThreadArgs* __restrict args = (ThreadArgs*)pArguments;
    HWND hOwnerWnd = args->hOwnerWnd;
    WorkQueue* __restrict queue = args->queue;

    ThreadLocalJarLists* __restrict tlsLists = (ThreadLocalJarLists*)calloc(1, sizeof(ThreadLocalJarLists));
    if (!tlsLists) { return 1; }

    InitMasterList(&tlsLists->allJarsList);
    InitMasterList(&tlsLists->mavenList);
    InitMasterList(&tlsLists->gradleList);
    InitMasterList(&tlsLists->forgeList);
    InitMasterList(&tlsLists->fabricList);
    InitMasterList(&tlsLists->mcpList);
    InitMasterList(&tlsLists->libsList);
    InitMasterList(&tlsLists->runnableJarsList);

    WCHAR currentPath[MAX_PATH];
    while (Queue_Pop(queue, currentPath)) {
        ProcessDirectory(currentPath, hOwnerWnd, SCAN_MODE_QUEUE, queue, tlsLists);
    }

    args->results = tlsLists;
    return 0;
}

unsigned __stdcall JarScanThread_Single(void* pArguments) {
    HWND hOwnerWnd = (HWND)pArguments;
    ClearAllJarLists();
    if (!InitializeNtApi()) {
        MessageBoxW(hOwnerWnd, L"Failed to initialize NT API functions.", L"Error", MB_ICONERROR);
        PostMessage(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
        return 1;
    }
    WCHAR driveStrings[MAX_PATH];
    GetLogicalDriveStringsW(MAX_PATH, driveStrings);
    WCHAR* pDrive = driveStrings;
    while (*pDrive) {
        ProcessDirectory(pDrive, hOwnerWnd, SCAN_MODE_RECURSIVE, NULL, NULL);
        pDrive += wcslen(pDrive) + 1;
    }
    PostMessage(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
    return 0;
}

unsigned __stdcall JarScanThread_Multi(void* pArguments) {
    HWND hOwnerWnd = (HWND)pArguments;
    ClearAllJarLists();
    if (!InitializeNtApi()) {
        MessageBoxW(hOwnerWnd, L"Failed to initialize NT API functions.", L"Error", MB_ICONERROR);
        PostMessage(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
        return 1;
    }
    WorkQueue queue;
    if (!Queue_Init(&queue)) {
        MessageBoxW(hOwnerWnd, L"Failed to allocate memory for work queue.", L"Error", MB_ICONERROR);
        PostMessage(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
        return 1;
    }

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numCores = sysInfo.dwNumberOfProcessors;
    DWORD numThreads = numCores * 2;

    HANDLE* hThreads = (HANDLE*)calloc(numThreads, sizeof(HANDLE));
    ThreadArgs* threadArgs = (ThreadArgs*)calloc(numThreads, sizeof(ThreadArgs));

    if (!hThreads || !threadArgs) {
        MessageBoxW(hOwnerWnd, L"Failed to allocate memory for thread handles or arguments.", L"Error", MB_ICONERROR);
        free(hThreads); free(threadArgs); Queue_Destroy(&queue);
        PostMessage(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
        return 1;
    }

    for (DWORD i = 0; i < numThreads; i++) {
        threadArgs[i].hOwnerWnd = hOwnerWnd;
        threadArgs[i].queue = &queue;
        threadArgs[i].results = NULL;
        hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, &ScanWorkerThread, &threadArgs[i], 0, NULL);

        if (hThreads[i] && numCores > 0) {
            DWORD_PTR affinityMask = (DWORD_PTR)1 << (i % numCores);
            SetThreadAffinityMask(hThreads[i], affinityMask);
        }
    }

    WCHAR userProfilePath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, userProfilePath))) {
        Queue_Push(&queue, userProfilePath);
    }
    WCHAR driveStrings[MAX_PATH];
    GetLogicalDriveStringsW(MAX_PATH, driveStrings);
    WCHAR* pDrive = driveStrings;
    while (*pDrive) {
        Queue_Push(&queue, pDrive);
        pDrive += wcslen(pDrive) + 1;
    }

    InterlockedExchange(&queue.finished_seeding, TRUE);
    for (DWORD i = 0; i < numThreads; i++) {
        if (hThreads[i] != NULL) {
            WaitForSingleObject(hThreads[i], INFINITE);
        }
    }

    for (DWORD i = 0; i < numThreads; i++) {
        if (hThreads[i] != NULL) {
            CloseHandle(hThreads[i]);
        }
        ThreadLocalJarLists* tls = threadArgs[i].results;
        if (tls) {
            MergeThreadLocalLists(tls);
            free(tls->allJarsList.items);
            free(tls->mavenList.items);
            free(tls->gradleList.items);
            free(tls->forgeList.items);
            free(tls->fabricList.items);
            free(tls->mcpList.items);
            free(tls->libsList.items);
            free(tls->runnableJarsList.items);
            free(tls);
        }
    }
    free(hThreads);
    free(threadArgs);

    RepopulateAllJarListViewsFromMasterLists();

    Queue_Destroy(&queue);
    PostMessage(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
    return 0;
}


unsigned __stdcall GlobalClassScanThread(void* __restrict pArguments) {
    GlobalSearchThreadData* __restrict threadData = (GlobalSearchThreadData*)pArguments;
    if (!threadData) return 1;

    HWND hSearchWnd = threadData->hSearchWnd;
    int totalMatches = 0;

    MasterList privateJarList;
    InitMasterList(&privateJarList);

    EnterCriticalSection(&g_listLock);
    for (int j = 0; j < g_allJarsMasterList.count; j++) {
        FileInfo* pJarInfo = (FileInfo*)g_allJarsMasterList.items[j];
        WCHAR* pathCopy = _wcsdup(pJarInfo->szFilePath);
        if (pathCopy) AddToMasterList(&privateJarList, pathCopy);
    }
    LeaveCriticalSection(&g_listLock);

    for (int i = 0; i < privateJarList.count; i++) {
        WCHAR* jarPath = (WCHAR*)privateJarList.items[i];
        int matchCountInJar = 0;

        FILE* file;
        if (_wfopen_s(&file, jarPath, L"rb") == 0 && file) {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            if (fileSize < sizeof(EOCDRecord)) {
                fclose(file);
                continue;
            }

            EOCDRecord eocd = { 0 };
            BOOL foundEOCD = FALSE;

            long readSize = min(fileSize, 65535 + sizeof(EOCDRecord));
            char* eocdSearchBuffer = (char*)malloc(readSize);
            if (eocdSearchBuffer) {
                fseek(file, fileSize - readSize, SEEK_SET);
                if (fread(eocdSearchBuffer, 1, readSize, file) == readSize) {
                    for (char* p = eocdSearchBuffer + readSize - sizeof(EOCDRecord); p >= eocdSearchBuffer; --p) {
                        if (*(uint32_t*)p == 0x06054b50) {
                            memcpy(&eocd, p, sizeof(eocd));
                            foundEOCD = TRUE;
                            break;
                        }
                    }
                }
                free(eocdSearchBuffer);
            }


            if (foundEOCD && eocd.dirSize > 0 && eocd.dirOffset + eocd.dirSize <= (unsigned long)fileSize) {
                char* centralDirBuffer = (char*)malloc(eocd.dirSize);
                if (centralDirBuffer) {
                    fseek(file, eocd.dirOffset, SEEK_SET);
                    if (fread(centralDirBuffer, 1, eocd.dirSize, file) == eocd.dirSize) {
                        char* p = centralDirBuffer;
                        for (UINT entryIdx = 0; entryIdx < eocd.totalEntries; ++entryIdx) {
                            if (p + sizeof(CDHeader) > centralDirBuffer + eocd.dirSize) break;
                            CDHeader* header = (CDHeader*)p;
                            if (header->sig != 0x02014b50) break;

                            DWORD entryDiskSize = sizeof(CDHeader) + header->nameLen + header->extraLen + header->cmtLen;
                            if (p + entryDiskSize > centralDirBuffer + eocd.dirSize) break;

                            if (header->nameLen > 0 && header->nameLen < MAX_PATH) {
                                char filename[MAX_PATH] = { 0 };
                                __movsb((unsigned char*)filename, (const unsigned char*)p + sizeof(CDHeader), header->nameLen);
                                filename[header->nameLen] = '\0';

                                if (strstr(filename, ".class")) {
                                    WCHAR wFilename[MAX_PATH];
                                    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wFilename, MAX_PATH);
                                    if (threadData->searchTerm[0] == L'\0' || StrStrIW(wFilename, threadData->searchTerm)) {
                                        matchCountInJar++;
                                    }
                                }
                            }
                            p += entryDiskSize;
                        }
                    }
                    free(centralDirBuffer);
                }
            }
            fclose(file);
        }

        if (matchCountInJar > 0) {
            JarWithMatches* jarInfo = (JarWithMatches*)malloc(sizeof(JarWithMatches));
            if (jarInfo) {
                wcscpy_s(jarInfo->jarPath, MAX_PATH, jarPath);
                jarInfo->matchCount = matchCountInJar;

                EnterCriticalSection(&g_listLock);
                AddToMasterList(&g_globalSearchMasterList, jarInfo);
                LeaveCriticalSection(&g_listLock);
                totalMatches += matchCountInJar;
            }
        }
    }

    for (int i = 0; i < privateJarList.count; i++) free(privateJarList.items[i]);
    free(privateJarList.items);

    PostMessageW(hSearchWnd, WM_APP_GLOBAL_SEARCH_COMPLETE, (WPARAM)totalMatches, 0);
    free(threadData);
    return 0;
}