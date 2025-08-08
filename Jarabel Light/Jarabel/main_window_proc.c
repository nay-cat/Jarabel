#include "main_window_proc.h"
#include "core/app.h"
#include "threads.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_isFastScanEnabled = FALSE;
        CreateUI(hwnd);
        break;

    case WM_COMMAND: {
        WORD identifier = LOWORD(wParam);
        WORD notificationCode = HIWORD(wParam);

        if (notificationCode == EN_CHANGE) {
            RepopulateActiveList();
        }
        else if (identifier == ID_SORT_COMBO || identifier == ID_FILTER_COMBO) {
            if (notificationCode == CBN_SELCHANGE) {
                int selectedSort = ComboBox_GetCurSel(hSortComboBox);
                int iPage = TabCtrl_GetCurSel(hTab);
                MasterList* pList = NULL;

                switch (iPage) {
                case 1: pList = &g_mavenMasterList; break;
                case 2: pList = &g_gradleMasterList; break;
                case 3: pList = &g_forgeMasterList; break;
                case 4: pList = &g_fabricMasterList; break;
                case 5: pList = &g_mcpMasterList; break;
                case 6: pList = &g_libsMasterList; break;
                case 7: pList = &g_runnableJarsMasterList; break;
                case 8: pList = &g_classesMasterList; break;
                case 10: pList = &g_recentMasterList; break;
                case 11: pList = &g_prefetchMasterList; break;
                case 12: pList = &g_processesMasterList; break;
                }

                if (pList && pList->count > 0) {
                    EnterCriticalSection(&g_listLock);
                    switch (selectedSort) {
                    case 0: qsort(pList->items, pList->count, sizeof(void*), CompareBySizeAsc); break;
                    case 1: qsort(pList->items, pList->count, sizeof(void*), CompareBySizeDesc); break;
                    case 2: qsort(pList->items, pList->count, sizeof(void*), CompareByDateAsc); break;
                    case 3: qsort(pList->items, pList->count, sizeof(void*), CompareByDateDesc); break;
                    }
                    LeaveCriticalSection(&g_listLock);
                }
                RepopulateActiveList();
            }
        }

        switch (identifier) {
        case ID_BUTTON_JAR_SCAN:
        {
            uintptr_t hThread = 0;
            if (g_isFastScanEnabled) {
                hThread = _beginthreadex(NULL, 0, &UnstableJarScanThread, hwnd, 0, NULL);
            }
            else {
                hThread = _beginthreadex(NULL, 0, &StableJarScanThread, hwnd, 0, NULL);
            }

            if (hThread) {
                CloseHandle((HANDLE)(hThread));
            }
            else {
                EnableWindow(hJarScanButton, TRUE);
                SetWindowTextW(hJarScanButton, L"Scan JARs");
                MessageBoxW(hwnd, L"Failed to start scan thread.", L"Error", MB_ICONERROR);
                break;
            }

            EnableWindow(hJarScanButton, FALSE);
            SetWindowTextW(hJarScanButton, L"Scanning...");
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            break;
        }
        case ID_BUTTON_FAST_SCAN:
            g_isFastScanEnabled = !g_isFastScanEnabled;
            InvalidateRect(hFastScanButton, NULL, TRUE);
            break;
        case ID_BUTTON_GLOBAL_CLASS_SEARCH:
        {
            int totalJars = g_allJarsMasterList.count;
            if (totalJars == 0) {
                MessageBoxW(hwnd, L"Please run a full system JAR scan first to populate the JAR lists.", L"Scan Required", MB_ICONINFORMATION);
                return 0;
            }
            HWND hSearchWnd = CreateWindowW(L"GlobalSearchWindowClass", L"Global Class Search",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                800, 600, hwnd, NULL, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            ShowWindow(hSearchWnd, SW_SHOW);
            UpdateWindow(hSearchWnd);
        }
        break;

        case ID_BUTTON_OPEN: HandleJarInspect(hwnd); break;
        case ID_BUTTON_JOURNAL: HandleJournalCheck(hwnd); break;
        case ID_BUTTON_RECENT: HandleRecentDocsCheck(hwnd); break;
        case ID_BUTTON_PREFETCH: HandlePrefetchCheck(hwnd); break;
        case ID_BUTTON_PROCESS_SCAN: HandleProcessScan(hwnd); break;
        case ID_MENU_TOGGLE_DARK_MODE: ToggleDarkMode(hwnd); break;
        case ID_MENU_COPY_NAME:
        case ID_MENU_COPY_PATH:
        case ID_MENU_COPY_METHODS:
        case ID_MENU_COPY_ALL:
        {
            if (g_clickedItemIndex != -1 && g_hClickedListView != NULL) {
                WCHAR buffer[2048];
                size_t chars_to_copy = 0;

                if (g_hClickedListView == hJournalList) {
                    if (g_clickedItemIndex < g_journalMasterList.count) {
                        UsnJournalEntry* pEntry = (UsnJournalEntry*)g_journalMasterList.items[g_clickedItemIndex];
                        if (pEntry) {
                            switch (identifier) {
                            case ID_MENU_COPY_NAME:
                                chars_to_copy = wcsnlen_s(pEntry->fileName, MAX_PATH);
                                if (chars_to_copy > 0 && chars_to_copy < _countof(buffer)) {
                                    wcscpy_s(buffer, _countof(buffer), pEntry->fileName);
                                }
                                break;
                            case ID_MENU_COPY_ALL:
                            {
                                int written = swprintf_s(buffer, _countof(buffer), L"Drive: %s\r\nFile Name: %s\r\nReason: %s\r\nTimestamp: %s",
                                    pEntry->drive, pEntry->fileName, pEntry->reason, pEntry->timestamp);
                                if (written > 0) {
                                    chars_to_copy = written;
                                }
                            }
                            break;
                            }
                        }
                    }
                }
                else {
                    MasterList* pList = NULL;
                    if (g_hClickedListView == hMavenList) pList = &g_mavenMasterList;
                    else if (g_hClickedListView == hGradleList) pList = &g_gradleMasterList;
                    else if (g_hClickedListView == hForgeList) pList = &g_forgeMasterList;
                    else if (g_hClickedListView == hFabricList) pList = &g_fabricMasterList;
                    else if (g_hClickedListView == hMcpList) pList = &g_mcpMasterList;
                    else if (g_hClickedListView == hLibsList) pList = &g_libsMasterList;
                    else if (g_hClickedListView == hRunnableJarsList) pList = &g_runnableJarsMasterList;
                    else if (g_hClickedListView == hJarList) pList = &g_classesMasterList;
                    else if (g_hClickedListView == hRecentList) pList = &g_recentMasterList;
                    else if (g_hClickedListView == hPrefetchList) pList = &g_prefetchMasterList;
                    else if (g_hClickedListView == hProcessList) pList = &g_processesMasterList;

                    if (pList && g_clickedItemIndex < pList->count) {
                        FileInfo* pInfo = (FileInfo*)pList->items[g_clickedItemIndex];
                        if (pInfo) {
                            switch (identifier) {
                            case ID_MENU_COPY_NAME:
                            {
                                const WCHAR* filename = PathFindFileNameW(pInfo->szFilePath);
                                chars_to_copy = wcsnlen_s(filename, MAX_PATH);
                                if (chars_to_copy > 0 && chars_to_copy < _countof(buffer)) {
                                    wcscpy_s(buffer, _countof(buffer), filename);
                                }
                            }
                            break;
                            case ID_MENU_COPY_PATH:
                            case ID_MENU_COPY_METHODS:
                            {
                                chars_to_copy = wcsnlen_s(pInfo->szFilePath, MAX_PATH);
                                if (chars_to_copy > 0 && chars_to_copy < _countof(buffer)) {
                                    wcscpy_s(buffer, _countof(buffer), pInfo->szFilePath);
                                }
                            }
                            break;
                            case ID_MENU_COPY_ALL:
                            {
                                WCHAR szSize[64], szTime[64];
                                FormatFileSize(pInfo->liFileSize, szSize, _countof(szSize));
                                FormatFileTime(pInfo->ftLastAccessTime, szTime, _countof(szTime));
                                int written = swprintf_s(buffer, _countof(buffer), L"Name: %s\r\nPath: %s\r\nSize: %s\r\nLast Used: %s",
                                    PathFindFileNameW(pInfo->szFilePath), pInfo->szFilePath, szSize, szTime);
                                if (written > 0) {
                                    chars_to_copy = written;
                                }
                            }
                            break;
                            }
                        }
                    }
                }

                if (chars_to_copy > 0) {
                    CopyToClipboard(hwnd, buffer, chars_to_copy);
                }
            }
        }
        break;
        }
        break;
    }

    case WM_APP_ADD_JAR_SCAN_ITEM: {
        EnterCriticalSection(&g_listLock);
        FileInfo* pInfo = (FileInfo*)lParam;
        if (pInfo) {
            AddToMasterList(&g_allJarsMasterList, pInfo);

            const int filterType = ComboBox_GetCurSel(hFilterComboBox);
            const bool shouldDisplay = (filterType == 0) || (filterType == 1 && pInfo->isObfuscated);

            LVITEMW lvi = { 0 };
            lvi.mask = LVIF_PARAM;
            lvi.lParam = (LPARAM)pInfo;

            if (pInfo->type & JAR_TYPE_MAVEN) {
                AddToMasterList(&g_mavenMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hMavenList); ListView_InsertItem(hMavenList, &lvi); }
            }
            if (pInfo->type & JAR_TYPE_GRADLE) {
                AddToMasterList(&g_gradleMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hGradleList); ListView_InsertItem(hGradleList, &lvi); }
            }
            if (pInfo->type & JAR_TYPE_FORGE) {
                AddToMasterList(&g_forgeMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hForgeList); ListView_InsertItem(hForgeList, &lvi); }
            }
            if (pInfo->type & JAR_TYPE_FABRIC) {
                AddToMasterList(&g_fabricMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hFabricList); ListView_InsertItem(hFabricList, &lvi); }
            }
            if (pInfo->type & JAR_TYPE_MCP) {
                AddToMasterList(&g_mcpMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hMcpList); ListView_InsertItem(hMcpList, &lvi); }
            }
            if (pInfo->type & JAR_TYPE_LIBS) {
                AddToMasterList(&g_libsMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hLibsList); ListView_InsertItem(hLibsList, &lvi); }
            }
            if (pInfo->type & JAR_TYPE_RUNNABLE) {
                AddToMasterList(&g_runnableJarsMasterList, pInfo);
                if (shouldDisplay) { lvi.iItem = ListView_GetItemCount(hRunnableJarsList); ListView_InsertItem(hRunnableJarsList, &lvi); }
            }
        }
        LeaveCriticalSection(&g_listLock);
        break;
    }
    case WM_APP_JAR_SCAN_COMPLETE: {
        SetWindowTextW(hJarScanButton, L"Scan All Drives");
        EnableWindow(hJarScanButton, TRUE);
        SetCursor(LoadCursor(NULL, IDC_ARROW));

        int total = g_allJarsMasterList.count;
        if (total == 0) {
            MessageBoxW(hwnd, L"No categorizable JAR files were found on any drive.", L"Scan Finished", MB_ICONINFORMATION);
        }
        else {
            MessageBoxW(hwnd, L"Full system JAR scan complete.", L"Scan Finished", MB_ICONINFORMATION);
        }
        break;
    }
    case WM_MOUSEWHEEL:
        HandleSmoothScroll(wParam);
        return 0;
    case WM_NOTIFY: {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->code == TCN_SELCHANGE) {
            HandleTabChange();
        }
        else if (lpnmh->code == NM_RCLICK) {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
            if (lpnmitem->iItem != -1) {
                g_clickedItemIndex = lpnmitem->iItem;
                g_hClickedListView = lpnmitem->hdr.hwndFrom;
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                HMENU hMenu = CreatePopupMenu();

                if (g_hClickedListView == hJournalList) {
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_NAME, L"Copy Name");
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_ALL, L"Copy All");
                }
                else {
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_NAME, L"Copy Name");
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_PATH, L"Copy Path");
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_METHODS, L"Copy Class Name");
                    AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_ALL, L"Copy All Info");
                    if (TabCtrl_GetCurSel(hTab) != 8) {
                        EnableMenuItem(hMenu, ID_MENU_COPY_METHODS, MF_BYCOMMAND | MF_GRAYED);
                    }
                }
                TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, cursorPos.x, cursorPos.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
        }
        else if (lpnmh->code == NM_CUSTOMDRAW) {
            LRESULT result;
            HandleCustomDraw(lParam, &result);
            return result;
        }
        break;
    }
    case WM_MEASUREITEM: return HandleMeasureItem(lParam);
    case WM_DRAWITEM: return HandleOwnerDraw(lParam);
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        if (g_isDarkMode) {
            return HandleCtlColor(msg, wParam);
        }
        break;
    }
    case WM_SIZE: HandleAppResize(lParam); break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}