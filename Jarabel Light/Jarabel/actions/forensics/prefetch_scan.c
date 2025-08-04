#include "scans.h"

static void InitializePrefetchScan() {
    EnterCriticalSection(&g_listLock);
    ClearMasterList(&g_prefetchMasterList);
    LeaveCriticalSection(&g_listLock);
    ListView_DeleteAllItems(hPrefetchList);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
}

static void AddPrefetchFileToList(FileInfo* pInfo, int* count) {
    EnterCriticalSection(&g_listLock);
    AddToMasterList(&g_prefetchMasterList, pInfo);
    LeaveCriticalSection(&g_listLock);

    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_PARAM;
    lvi.iItem = (*count)++;
    lvi.lParam = (LPARAM)pInfo;
    ListView_InsertItem(hPrefetchList, &lvi);
}

static void ScanPrefetchFolder(const WCHAR* prefetchPath, int* count, const WCHAR* windowsDir) {
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(prefetchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if (StrStrIW(findData.cFileName, L".JAR") != NULL || StrStrIW(findData.cFileName, L"JAVA.EXE") != NULL || StrStrIW(findData.cFileName, L"JAVAW.EXE") != NULL) {
                FileInfo* pInfo = (FileInfo*)malloc(sizeof(FileInfo));
                if (pInfo) {
                    swprintf_s(pInfo->szFilePath, MAX_PATH, L"%s\\Prefetch\\%s", windowsDir, findData.cFileName);
                    pInfo->liFileSize.LowPart = findData.nFileSizeLow;
                    pInfo->liFileSize.HighPart = findData.nFileSizeHigh;
                    pInfo->ftLastAccessTime = findData.ftLastWriteTime;
                    pInfo->isObfuscated = FALSE;
                    pInfo->entropy = 0;
                    AddPrefetchFileToList(pInfo, count);
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
}

static void FinalizePrefetchScan(HWND hwnd, int count) {
    if (count == 0) {
        MessageBoxW(hwnd, L"No Java-related prefetch files were found.", L"Scan Finished", MB_ICONINFORMATION);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void HandlePrefetchCheck(HWND hwnd) {
    InitializePrefetchScan(hwnd, hPrefetchList);
    WCHAR windowsDir[MAX_PATH];
    if (GetWindowsDirectoryW(windowsDir, MAX_PATH) == 0) {
        MessageBoxW(hwnd, L"Could not determine Windows directory.", L"Error", MB_ICONERROR);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    WCHAR prefetchPath[MAX_PATH];
    swprintf_s(prefetchPath, MAX_PATH, L"%s\\Prefetch\\*", windowsDir);
    int count = 0;
    ScanPrefetchFolder(prefetchPath, &count, windowsDir);
    FinalizePrefetchScan(hwnd, count);
}