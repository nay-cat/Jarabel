#include "../core/app.h"

static WCHAR g_cachedJarPath[MAX_PATH] = { 0 };
static MasterList g_cachedClassNames;
static BOOL g_cacheInitialized = FALSE;

static void ClearVirtualSearchCache() {
    if (g_cacheInitialized) {
        ClearMasterList(&g_cachedClassNames);
        free(g_cachedClassNames.items);
        g_cachedClassNames.items = NULL;
        wcscpy_s(g_cachedJarPath, MAX_PATH, L"");
        g_cacheInitialized = FALSE;
    }
}

static BOOL GetClassEntryForIndex(int globalIndex, ClassEntry* pEntry) {
    if (!pEntry || globalIndex < 0) return FALSE;

    EnterCriticalSection(&g_listLock);

    int cumulativeMatches = 0;
    JarWithMatches* targetJarInfo = NULL;
    int indexInJar = -1;

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

    if (wcscmp(g_cachedJarPath, targetJarInfo->jarPath) != 0) {
        ClearVirtualSearchCache();
        wcscpy_s(g_cachedJarPath, MAX_PATH, targetJarInfo->jarPath);
        InitMasterList(&g_cachedClassNames);
        g_cacheInitialized = TRUE;

        FILE* file;
        if (_wfopen_s(&file, g_cachedJarPath, L"rb") == 0 && file) {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            if (fileSize > sizeof(EOCDRecord)) {
                EOCDRecord eocd = { 0 };
                long searchPos = fileSize - sizeof(eocd);
                BOOL foundEOCD = FALSE;
                for (int k = 0; searchPos - k >= 0 && k < 65535; ++k) {
                    fseek(file, searchPos - k, SEEK_SET);
                    uint32_t signature;
                    if (fread(&signature, sizeof(signature), 1, file) == 1 && signature == 0x06054b50) {
                        fseek(file, searchPos - k, SEEK_SET);
                        if (fread(&eocd, sizeof(eocd), 1, file) == 1) { foundEOCD = TRUE; break; }
                    }
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
                                    char filename[MAX_PATH];
                                    memcpy_s(filename, MAX_PATH, p + sizeof(CDHeader), header->nameLen);
                                    filename[header->nameLen] = '\0';
                                    if (strstr(filename, ".class")) {
                                        char* nameCopy = _strdup(filename);
                                        if (nameCopy) AddToMasterList(&g_cachedClassNames, nameCopy);
                                    }
                                }
                                p += entryDiskSize;
                            }
                        }
                        free(centralDirBuffer);
                    }
                }
            }
            fclose(file);
        }
    }

    if (indexInJar >= 0 && indexInJar < g_cachedClassNames.count) {
        char* classNameAnsi = (char*)g_cachedClassNames.items[indexInJar];
        if (!classNameAnsi) return FALSE;

        size_t cchAnsi = strnlen_s(classNameAnsi, MAX_PATH);
        if (cchAnsi == 0 || cchAnsi >= MAX_PATH) return FALSE;

        int requiredChars = MultiByteToWideChar(CP_UTF8, 0, classNameAnsi, (int)cchAnsi, NULL, 0);

        if (requiredChars == 0 || (requiredChars + 1) > MAX_PATH) {
            return FALSE;
        }

        MultiByteToWideChar(CP_UTF8, 0, classNameAnsi, (int)cchAnsi, pEntry->className, MAX_PATH);

        pEntry->className[requiredChars] = L'\0';

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
        BOOL useDarkMode = TRUE;
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
        WORD identifier = LOWORD(wParam);
        if (identifier == ID_GLOBAL_SEARCH_BUTTON) {
            ClearVirtualSearchCache();
            EnterCriticalSection(&g_listLock);
            ClearMasterList(&g_globalSearchMasterList);
            LeaveCriticalSection(&g_listLock);
            ListView_SetItemCount(hList, 0);

            GlobalSearchThreadData* threadData = (GlobalSearchThreadData*)malloc(sizeof(GlobalSearchThreadData));
            if (threadData) {
                threadData->hSearchWnd = hwnd;
                GetWindowTextW(hSearch, threadData->searchTerm, 256);
                EnableWindow(hSearch, FALSE);
                EnableWindow(hSearchBtn, FALSE);
                _beginthreadex(NULL, 0, &GlobalClassScanThread, threadData, 0, NULL);
            }
        }
        else if (identifier == ID_MENU_COPY_CLASS_NAME) {
            if (s_nClickedItem != -1) {
                ClassEntry entry;
                if (GetClassEntryForIndex(s_nClickedItem, &entry)) {
                    size_t len = wcsnlen_s(entry.className, MAX_PATH);
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
                NMLVDISPINFOW* pdi = (NMLVDISPINFOW*)lParam;
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
                LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
                if (lpnmitem->iItem != -1) {
                    s_nClickedItem = lpnmitem->iItem;

                    POINT cursorPos;
                    GetCursorPos(&cursorPos);

                    HMENU hMenu = CreatePopupMenu();
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
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
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
        ClearMasterList(&g_globalSearchMasterList);
        LeaveCriticalSection(&g_listLock);
        DestroyWindow(hwnd);
        break;
    }

    default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}