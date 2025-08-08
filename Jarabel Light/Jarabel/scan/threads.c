#include "../core/app.h"
#include "threads.h"

static void JarFoundCallback(const WCHAR* path, void* context) {
    HWND hOwnerWnd = (HWND)context;
    if (!hOwnerWnd || !path) {
        return;
    }

    FileInfo* pInfo = (FileInfo*)malloc(sizeof(FileInfo));
    if (pInfo) {
        memset(pInfo, 0, sizeof(FileInfo));

        const HRESULT hr = StringCchCopyW(pInfo->szFilePath, MAX_PATH, path);
        if (FAILED(hr)) {
            free(pInfo);
            return;
        }

        AnalyzeAndCategorizeJar(pInfo, hOwnerWnd, NULL);
    }
}

unsigned __stdcall StableJarScanThread(void* __restrict pArguments) {
    HWND hOwnerWnd = (HWND)pArguments;

    ClearAllJarLists();
    RunStableScan(JarFoundCallback, hOwnerWnd);

    PostMessageW(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
    return 0;
}

unsigned __stdcall UnstableJarScanThread(void* __restrict pArguments) {
    HWND hOwnerWnd = (HWND)pArguments;

    ClearAllJarLists();
    RunUnstableScan(JarFoundCallback, hOwnerWnd);

    PostMessageW(hOwnerWnd, WM_APP_JAR_SCAN_COMPLETE, 0, 0);
    return 0;
}

static void ProcessGlobalSearchEntry(const CDHeader* header, const char* filename, FILE* file, void* context) {
    (void)file;
    (void)header;
    SearchContext* ctx = (SearchContext*)context;
    if (strstr(filename, ".class")) {
        WCHAR wFilename[MAX_PATH];
        if (Utf8ToWide(filename, -1, wFilename, MAX_PATH)) {
            if (ctx->searchTerm[0] == L'\0' || StrStrIW(wFilename, ctx->searchTerm)) {
                ctx->matchCount++;
            }
        }
    }
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

        SearchContext context = { 0 };
        context.searchTerm = threadData->searchTerm;

        ParseJarCentralDirectory(jarPath, ProcessGlobalSearchEntry, &context);

        if (context.matchCount > 0) {
            JarWithMatches* jarInfo = (JarWithMatches*)malloc(sizeof(JarWithMatches));
            if (jarInfo) {
                const HRESULT hr2 = StringCchCopyW(jarInfo->jarPath, MAX_PATH, jarPath);
                if (FAILED(hr2)) {
                    free(jarInfo);
                }
                else {
                    jarInfo->matchCount = context.matchCount;

                    EnterCriticalSection(&g_listLock);
                    AddToMasterList(&g_globalSearchMasterList, jarInfo);
                    LeaveCriticalSection(&g_listLock);
                    totalMatches += context.matchCount;
                }
            }
        }
    }

    for (int i = 0; i < privateJarList.count; i++) free(privateJarList.items[i]);
    free(privateJarList.items);

    PostMessageW(hSearchWnd, WM_APP_GLOBAL_SEARCH_COMPLETE, (WPARAM)totalMatches, 0);
    free(threadData);
    return 0;
}