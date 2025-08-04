#include "scans.h"

static void InitializeRecentDocsScan() {
    EnterCriticalSection(&g_listLock);
    ClearMasterList(&g_recentMasterList);
    LeaveCriticalSection(&g_listLock);

    // Disables redrawing to prevent flicker while adding a large number of items, althought recent should not have a very large number of items
    SendMessageW(hRecentList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hRecentList);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
}

static void AddRecentDocToList(FileInfo* __restrict pInfo, int* __restrict count) {
    EnterCriticalSection(&g_listLock);
    AddToMasterList(&g_recentMasterList, pInfo);
    LeaveCriticalSection(&g_listLock);

    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_PARAM;
    lvi.iItem = (*count)++;
    lvi.lParam = (LPARAM)pInfo;
    ListView_InsertItem(hRecentList, &lvi);
}

static void ProcessPathFromRegistryData(const BYTE* __restrict pData, DWORD cbData, int* __restrict count) {
    if (cbData < sizeof(WCHAR) * 4) return;

    for (DWORD i = 0; i <= cbData - (sizeof(WCHAR) * 4); i += sizeof(WCHAR)) {
        const WCHAR* path = (const WCHAR*)(pData + i);

        if (path[0] != L'\0' && path[1] == L':' && path[2] == L'\\') {
            size_t pathLen = 0;
            if (SUCCEEDED(StringCchLengthW(path, (cbData - i) / sizeof(WCHAR), &pathLen)) && pathLen > 0) {
                FileInfo* pInfo = (FileInfo*)calloc(1, sizeof(FileInfo));
                if (pInfo) {
                    StringCchCopyW(pInfo->szFilePath, MAX_PATH, path);

                    WIN32_FILE_ATTRIBUTE_DATA fad = { 0 };
                    if (GetFileAttributesExW(pInfo->szFilePath, GetFileExInfoStandard, &fad)) {
                        pInfo->liFileSize.LowPart = fad.nFileSizeLow;
                        pInfo->liFileSize.HighPart = fad.nFileSizeHigh;
                        pInfo->ftLastAccessTime = fad.ftLastAccessTime;
                    }
                    else {
                        pInfo->liFileSize.QuadPart = (ULONGLONG)-1;
                    }
                    AddRecentDocToList(pInfo, count);
                }
                break;
            }
        }
    }
}

static void ProcessRecentDocsRegistry(HKEY hKey, int* __restrict count) {
    DWORD dwIndex = 0;
    BYTE byData[4096];
    WCHAR wszValueName[MAX_PATH];

    while (TRUE) {
        DWORD cbData = sizeof(byData);
        DWORD cchValueName = MAX_PATH;
        LSTATUS status = RegEnumValueW(hKey, dwIndex++, wszValueName, &cchValueName, NULL, NULL, byData, &cbData);

        if (status == ERROR_NO_MORE_ITEMS) {
            break; 
        }
        if (status == ERROR_SUCCESS) {
            ProcessPathFromRegistryData(byData, cbData, count);
        }
    }
}

static void FinalizeRecentDocsScan(HWND hwnd, int count) {
    // basically re-enables redrawing and force a repaint to show all new items at once
    SendMessageW(hRecentList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hRecentList, NULL, TRUE);

    if (count == 0) {
        MessageBoxW(hwnd, L"No recently used .jar files were found in the registry.", L"Scan Finished", MB_ICONINFORMATION);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void HandleRecentDocsCheck(HWND hwnd) {
    InitializeRecentDocsScan(hRecentList);

    HKEY hKey;
    int count = 0;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs\\.jar", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        ProcessRecentDocsRegistry(hKey, &count);
        RegCloseKey(hKey);
    }

    FinalizeRecentDocsScan(hwnd, count);
}