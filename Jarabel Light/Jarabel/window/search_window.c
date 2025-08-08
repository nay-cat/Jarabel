#include "search_window.h"

static WCHAR g_cachedJarPath[MAX_PATH] = { 0 };
static MasterList g_cachedClassNames;
static bool g_cacheInitialized = FALSE;
static WCHAR g_currentSearchTerm[256] = { 0 }; 

static void ClearVirtualSearchCache() {
    if (g_cacheInitialized) {
        DestroyMasterList(&g_cachedClassNames); // free(g_cachedClassNames.items);
        wcscpy_s(g_cachedJarPath, MAX_PATH, L"");
        g_cacheInitialized = FALSE;
    }
}

static void PopulateCacheCallback(const CDHeader* header, const char* filename, FILE* file, void* context) {
    UNREFERENCED_PARAMETER(header); UNREFERENCED_PARAMETER(file);
    CachePopulationContext* ctx = (CachePopulationContext*)context;

    if (strstr(filename, ".class")) {
        WCHAR wFilename[MAX_PATH];
        if (Utf8ToWide(filename, -1, wFilename, MAX_PATH)) {
            if (g_currentSearchTerm[0] == L'\0' || StrStrIW(wFilename, g_currentSearchTerm)) {
                char* nameCopy = _strdup(filename);
                if (nameCopy) {
                    AddToMasterList(ctx->classNamesList, nameCopy);
                }
            }
        }
    }
}


static bool GetClassEntryForIndex(const int globalIndex, ClassEntry* pEntry) {
    if (!pEntry || globalIndex < 0) return FALSE;

    EnterCriticalSection(&g_listLock);

    int cumulativeMatches = 0;
    JarWithMatches* targetJarInfo = NULL;
    int indexInJar = -1;

    // find which JAR contains the globally-indexed item
    for (int i = 0; i < g_globalSearchMasterList.count; i++) {
        JarWithMatches* currentJarInfo = (JarWithMatches*)g_globalSearchMasterList.items[i];
        if (globalIndex < cumulativeMatches + currentJarInfo->matchCount) {
            targetJarInfo = currentJarInfo;
            indexInJar = globalIndex - cumulativeMatches;
            break;
        }
        cumulativeMatches += currentJarInfo->matchCount;
    }

    LeaveCriticalSection(&g_listLock);

    if (!targetJarInfo) return FALSE;

    // because if the requested JAR is not the one we have cached, we must re-populate the cache
    if (wcscmp(g_cachedJarPath, targetJarInfo->jarPath) != 0) {
        ClearVirtualSearchCache();
        wcscpy_s(g_cachedJarPath, MAX_PATH, targetJarInfo->jarPath);
        InitMasterList(&g_cachedClassNames);
        g_cacheInitialized = TRUE;

        CachePopulationContext context = { 0 };
        context.classNamesList = &g_cachedClassNames;
        ParseJarCentralDirectory(g_cachedJarPath, PopulateCacheCallback, &context);
    }

    if (indexInJar >= 0 && indexInJar < g_cachedClassNames.count) {
        char* classNameAnsi = (char*)g_cachedClassNames.items[indexInJar];
        if (!classNameAnsi) {
            return FALSE;
        }

        if (Utf8ToWide(classNameAnsi, -1, pEntry->className, MAX_PATH) == 0) {
            return FALSE;
        }

        wcscpy_s(pEntry->parentJar, MAX_PATH, PathFindFileNameW(targetJarInfo->jarPath));
        return TRUE;
    }

    return FALSE;
}

LRESULT CALLBACK GlobalSearchWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hSearch, hList, hSearchBtn;
    static int s_nClickedItem = -1;

    switch (msg) {
    case WM_CREATE: {
        bool useDarkMode = TRUE;
        DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));

        hSearch = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 10, 675, 25, hwnd, (HMENU)ID_GLOBAL_SEARCH_EDIT, NULL, NULL);
        hSearchBtn = CreateWindowW(L"BUTTON", L"Search", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            690, 10, 80, 25, hwnd, (HMENU)ID_GLOBAL_SEARCH_BUTTON, NULL, NULL);

        hList = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA,
            10, 40, 760, 500, hwnd, (HMENU)ID_GLOBAL_SEARCH_LIST, NULL, NULL);

        ListView_SetBkColor(hList, g_darkBgColor);
        ListView_SetTextBkColor(hList, g_darkBgColor);
        ListView_SetTextColor(hList, g_darkTextColor);
        SubclassListViewHeader(hList);

        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
        LV_COLUMNW lvc = { 0 };
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;
        lvc.pszText = L"Class Name"; lvc.cx = 400; ListView_InsertColumn(hList, 0, &lvc);
        lvc.pszText = L"Parent JAR"; lvc.cx = 350; ListView_InsertColumn(hList, 1, &lvc);
        break;
    }

    case WM_COMMAND: {
        const WORD identifier = LOWORD(wParam);
        if (identifier == ID_GLOBAL_SEARCH_BUTTON) {
            ClearVirtualSearchCache();
            EnterCriticalSection(&g_listLock);
            DestroyMasterList(&g_globalSearchMasterList);
            LeaveCriticalSection(&g_listLock);
            ListView_SetItemCount(hList, 0);

            GlobalSearchThreadData* threadData = (GlobalSearchThreadData*)malloc(sizeof(GlobalSearchThreadData));
            if (threadData) {
                threadData->hSearchWnd = hwnd;
                GetWindowTextW(hSearch, threadData->searchTerm, 256);

                wcscpy_s(g_currentSearchTerm, 256, threadData->searchTerm);

                EnableWindow(hSearch, FALSE);
                EnableWindow(hSearchBtn, FALSE);
                _beginthreadex(NULL, 0, &GlobalClassScanThread, threadData, 0, NULL);
            }
        }
        else if (identifier == ID_MENU_COPY_CLASS_NAME) {
            if (s_nClickedItem != -1) {
                ClassEntry entry = { 0 };
                if (GetClassEntryForIndex(s_nClickedItem, &entry)) {
                    const size_t len = wcsnlen_s(entry.className, MAX_PATH);
                    if (len > 0 && len < MAX_PATH) {
                        CopyToClipboard(hwnd, entry.className, len);
                    }
                }
            }
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->idFrom == ID_GLOBAL_SEARCH_LIST) {
            if (lpnmh->code == LVN_GETDISPINFO) {
                const NMLVDISPINFOW* pdi = (NMLVDISPINFOW*)lParam;
                static ClassEntry tempEntry;

                if (GetClassEntryForIndex(pdi->item.iItem, &tempEntry)) {
                    if (pdi->item.mask & LVIF_TEXT) {
                        if (pdi->item.iSubItem == 0)
                            wcscpy_s(pdi->item.pszText, pdi->item.cchTextMax, tempEntry.className);
                        else if (pdi->item.iSubItem == 1)
                            wcscpy_s(pdi->item.pszText, pdi->item.cchTextMax, tempEntry.parentJar);
                    }
                }
                else {
                    wcscpy_s(pdi->item.pszText, pdi->item.cchTextMax, L"Error loading...");
                }
                return TRUE;
            }
            else if (lpnmh->code == NM_RCLICK) {
                const LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
                if (lpnmitem->iItem != -1) {
                    s_nClickedItem = lpnmitem->iItem;

                    POINT cursorPos;
                    GetCursorPos(&cursorPos);

                    const HMENU hMenu = CreatePopupMenu();
                    if (hMenu) {
                        AppendMenuW(hMenu, MF_STRING, ID_MENU_COPY_CLASS_NAME, L"Copy Class Name");
                        TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                            cursorPos.x, cursorPos.y, 0, hwnd, NULL);
                        DestroyMenu(hMenu);
                    }
                }
            }
        }
        break;
    }

    case WM_APP_GLOBAL_SEARCH_COMPLETE: {
        EnableWindow(hSearch, TRUE);
        EnableWindow(hSearchBtn, TRUE);
        ListView_SetItemCountEx(hList, (int)wParam, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
        RedrawWindow(hList, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        break;
    }

    case WM_SIZE: {
        const int width = LOWORD(lParam);
        const int height = HIWORD(lParam);
        MoveWindow(hSearch, 10, 10, width - 115, 25, TRUE);
        MoveWindow(hSearchBtn, width - 100, 10, 80, 25, TRUE);
        MoveWindow(hList, 10, 40, width - 20, height - 55, TRUE);
        break;
    }

    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        if (g_isDarkMode) return HandleCtlColor(msg, wParam);
        break;
    }

    case WM_CLOSE: {
        ClearVirtualSearchCache();
        EnterCriticalSection(&g_listLock);
        DestroyMasterList(&g_globalSearchMasterList);
        LeaveCriticalSection(&g_listLock);
        DestroyWindow(hwnd);
        break;
    }

    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}