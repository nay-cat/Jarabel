#include "scans.h"

static void InitializeRecentDocsScan() {
    EnterCriticalSection(&g_listLock);
    DestroyMasterList(&g_recentMasterList);
    InitMasterList(&g_recentMasterList);
    LeaveCriticalSection(&g_listLock);

    // disables redrawing to prevent flicker while adding a large number of items
    SendMessageW(hRecentList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hRecentList);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
}

static void AddRecentDocToList(FileInfo* __restrict pInfo, int* __restrict pCount) {
    EnterCriticalSection(&g_listLock);
    AddToMasterList(&g_recentMasterList, pInfo);
    LeaveCriticalSection(&g_listLock);

    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_PARAM;
    lvi.iItem = (*pCount)++;
    lvi.lParam = (LPARAM)pInfo;
    ListView_InsertItem(hRecentList, &lvi);
}

static bool IsPathAlreadyInList(const WCHAR* szFilePath) {
    EnterCriticalSection(&g_listLock);
    bool bFound = FALSE;
    for (int i = 0; i < g_recentMasterList.count; i++) {
        FileInfo* pInfo = (FileInfo*)g_recentMasterList.items[i];
        if (pInfo && _wcsicmp(pInfo->szFilePath, szFilePath) == 0) {
            bFound = TRUE;
            break;
        }
    }
    LeaveCriticalSection(&g_listLock);
    return bFound;
}

static void ProcessRecentDocEntry(const BYTE* __restrict pData, DWORD cbData, int* __restrict pCount) {
    // the binary data in the registry is a UTF-16LE string (WCHAR* on Windows)
    if (pData == NULL || cbData < sizeof(WCHAR) * 4) return;
    const WCHAR* path = (const WCHAR*)pData;

    // this is to ensure it looks like a valid absolute path
    if (path[0] != L'\0' && path[1] == L':' && path[2] == L'\\') {
        size_t pathLen = 0;
        if (SUCCEEDED(StringCchLengthW(path, cbData / sizeof(WCHAR), &pathLen)) && pathLen > 0) {
            if (!IsPathAlreadyInList(path)) {
                FileInfo* pInfo = (FileInfo*)calloc(1, sizeof(FileInfo));
                if (pInfo) {
                    StringCchCopyW(pInfo->szFilePath, MAX_PATH, path);
                    PopulateFileInfo(pInfo);
                    AddRecentDocToList(pInfo, pCount);
                }
            }
        }
    }
}

static void ProcessRecentDocsRegistry(HKEY hKey, int* __restrict pCount) {
    DWORD dwIndex = 0;
    BYTE byData[4096];
    WCHAR wszValueName[MAX_PATH];

    while (TRUE) {
        DWORD cbData = sizeof(byData);
        DWORD cchValueName = MAX_PATH;
        DWORD dwType;
        if (RegEnumValueW(hKey, dwIndex++, wszValueName, &cchValueName, NULL, &dwType, byData, &cbData) != ERROR_SUCCESS) {
            break;
        }
        if (dwType == REG_BINARY) {
            ProcessRecentDocEntry(byData, cbData, pCount);
        }
    }
}

static void ScanRecentItemsFolderForLnk(int* pCount) {
    WCHAR szRecentPath[MAX_PATH];

    // this works regardless of the username or if the folder has been redirected, so we dont have to use COM
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_RECENT, NULL, 0, szRecentPath))) {
        return;
    }

    WCHAR szSearchPath[MAX_PATH];
    // because PathCombineW is pragma deprecated
    if (FAILED(PathCchCombine(szSearchPath, MAX_PATH, szRecentPath, L"*.lnk"))) {
        return;
    }

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(szSearchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        // normal if the folder is empty or doesn't exist
        return;
    }

    do {
        if (PathMatchSpecW(findData.cFileName, L"*.jar.lnk")) {
            WCHAR szLnkPath[MAX_PATH];
            if (SUCCEEDED(PathCchCombine(szLnkPath, MAX_PATH, szRecentPath, findData.cFileName))) {
                if (!IsPathAlreadyInList(szLnkPath)) {
                    FileInfo* pInfo = (FileInfo*)calloc(1, sizeof(FileInfo));
                    if (pInfo) {
                        StringCchCopyW(pInfo->szFilePath, MAX_PATH, szLnkPath);
                        PopulateFileInfo(pInfo);
                        AddRecentDocToList(pInfo, pCount);
                    }
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

static void FinalizeRecentDocsScan(HWND hwnd, int count) {
    // reenable redrawing and force a repaint to show all new items at once
    SendMessageW(hRecentList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hRecentList, NULL, TRUE);

    if (count == 0) {
        MessageBoxW(hwnd, L"No recently used .jar files were found in the registry or recent items folder.", L"Scan Finished", MB_ICONINFORMATION);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void HandleRecentDocsCheck(HWND hwnd) {
    InitializeRecentDocsScan();

    int count = 0;

    // registry check
    HKEY hKey;
    const WCHAR* szKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs\\.jar";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, szKeyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        ProcessRecentDocsRegistry(hKey, &count);
        RegCloseKey(hKey);
    }

    // filesystem check
    ScanRecentItemsFolderForLnk(&count);

    FinalizeRecentDocsScan(hwnd, count);
}