#include "../core/app.h"

void RepopulateActiveList() {
    int iPage = TabCtrl_GetCurSel(hTab);
    int filterType = ComboBox_GetCurSel(hFilterComboBox);
    BOOL isUsn = FALSE;

    MasterList* pList = NULL;
    HWND hList = NULL, hSearch = NULL;

    switch (iPage) {
    case 1: pList = &g_mavenMasterList; hList = hMavenList; hSearch = hSearchMaven; break;
    case 2: pList = &g_gradleMasterList; hList = hGradleList; hSearch = hSearchGradle; break;
    case 3: pList = &g_forgeMasterList; hList = hForgeList; hSearch = hSearchForge; break;
    case 4: pList = &g_fabricMasterList; hList = hFabricList; hSearch = hSearchFabric; break;
    case 5: pList = &g_mcpMasterList; hList = hMcpList; hSearch = hSearchMcp; break;
    case 6: pList = &g_libsMasterList; hList = hLibsList; hSearch = hSearchLibs; break;
    case 7: pList = &g_runnableJarsMasterList; hList = hRunnableJarsList; hSearch = hSearchRunnableJars; break;
    case 8: pList = &g_classesMasterList; hList = hJarList; hSearch = hSearchClasses; filterType = 0; break;
    case 9: pList = &g_journalMasterList; hList = hJournalList; hSearch = hSearchJournal; isUsn = TRUE; filterType = 0; break;
    case 10: pList = &g_recentMasterList; hList = hRecentList; hSearch = hSearchRecent; filterType = 0; break;
    case 11: pList = &g_prefetchMasterList; hList = hPrefetchList; hSearch = hSearchPrefetch; filterType = 0; break;
    case 12: pList = &g_processesMasterList; hList = hProcessList; hSearch = hSearchProcesses; filterType = 0; break;
    }

    if (pList) {
        FilterAndRepopulateListView(hSearch, hList, pList, isUsn, filterType);
    }
}


void HandleAppResize(LPARAM lParam) {
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);

    MoveWindow(hTab, 0, 0, width, height, TRUE);

    const int topMargin = 35;
    const int controlHeight = 25;
    const int spacing = 5;
    const int listTop = topMargin + controlHeight + spacing;

    int center_x = width / 2;

    MoveWindow(hTitleLabel, 0, topMargin + 20, width, 70, TRUE);
    MoveWindow(hDescLabel, center_x - 250, topMargin + 100, 500, 80, TRUE);
    MoveWindow(hJarScanButton, center_x - 155, topMargin + 190, 150, 30, TRUE);
    MoveWindow(hGlobalClassSearchButton, center_x + 5, topMargin + 190, 150, 30, TRUE);
    MoveWindow(hMultiThreadCheckBox, center_x - 155, topMargin + 230, 310, 25, TRUE);
    MoveWindow(hNijikaImage, center_x - 125, height - 300, 250, 250, TRUE);

    MoveWindow(hSortComboBox, 10, topMargin, 180, 200, TRUE);
    MoveWindow(hFilterComboBox, 10 + 180 + spacing, topMargin, 180, 200, TRUE);

    int searchLeft = 10 + 180 + spacing + 180 + spacing;
    int searchWidth = width - searchLeft - 15;

    MoveWindow(hSearchMaven, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hMavenList, 10, listTop, width - 25, height - listTop - 15, TRUE);
    MoveWindow(hSearchGradle, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hGradleList, 10, listTop, width - 25, height - listTop - 15, TRUE);
    MoveWindow(hSearchForge, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hForgeList, 10, listTop, width - 25, height - listTop - 15, TRUE);
    MoveWindow(hSearchFabric, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hFabricList, 10, listTop, width - 25, height - listTop - 15, TRUE);
    MoveWindow(hSearchMcp, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hMcpList, 10, listTop, width - 25, height - listTop - 15, TRUE);
    MoveWindow(hSearchLibs, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hLibsList, 10, listTop, width - 25, height - listTop - 15, TRUE);
    MoveWindow(hSearchRunnableJars, searchLeft, topMargin, searchWidth, 22, TRUE);
    MoveWindow(hRunnableJarsList, 10, listTop, width - 25, height - listTop - 15, TRUE);

    MoveWindow(hJarButton, 200, topMargin, 120, controlHeight, TRUE);
    MoveWindow(hSearchClasses, 325, topMargin, width - 530, controlHeight, TRUE);
    MoveWindow(hEntropyLabel, width - 200, topMargin, 175, controlHeight, TRUE);
    MoveWindow(hJarList, 10, listTop, width - 25, height - listTop - 15, TRUE);

    MoveWindow(hJournalButton, 10, topMargin, 180, controlHeight, TRUE);
    MoveWindow(hSearchJournal, 195, topMargin, width - 210, 22, TRUE);
    MoveWindow(hJournalList, 10, listTop, width - 25, height - listTop - 15, TRUE);

    MoveWindow(hRecentButton, 200, topMargin, 180, controlHeight, TRUE);
    MoveWindow(hSearchRecent, 390, topMargin, width - 405, 22, TRUE);
    MoveWindow(hRecentList, 10, listTop, width - 25, height - listTop - 15, TRUE);

    MoveWindow(hPrefetchButton, 200, topMargin, 180, controlHeight, TRUE);
    MoveWindow(hSearchPrefetch, 390, topMargin, width - 405, 22, TRUE);
    MoveWindow(hPrefetchList, 10, listTop, width - 25, height - listTop - 15, TRUE);

    MoveWindow(hProcessScanButton, 200, topMargin, 220, controlHeight, TRUE);
    MoveWindow(hSearchProcesses, 430, topMargin, width - 445, 22, TRUE);
    MoveWindow(hProcessList, 10, listTop, width - 25, height - listTop - 15, TRUE);
}

void ToggleDarkMode(HWND hwnd) {
    g_isDarkMode = !g_isDarkMode;

    SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)(g_isDarkMode ? g_darkBrush : g_lightBrush));

    DwmSetWindowAttribute(hwnd, 20, &g_isDarkMode, sizeof(g_isDarkMode));

    COLORREF bkColor = g_isDarkMode ? g_darkBgColor : GetSysColor(COLOR_WINDOW);

    HWND allListViews[] = {
        hMavenList, hGradleList, hForgeList, hFabricList, hMcpList, hLibsList,
        hRunnableJarsList, hJarList, hJournalList, hRecentList, hPrefetchList, hProcessList
    };
    int numListViews = sizeof(allListViews) / sizeof(HWND);

    for (int i = 0; i < numListViews; ++i) {
        if (allListViews[i]) {
            ListView_SetBkColor(allListViews[i], bkColor);
            HWND hHeader = ListView_GetHeader(allListViews[i]);
            if (hHeader) {
                RedrawWindow(hHeader, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
        }
    }

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

void HandleTabChange() {
    int iPage = TabCtrl_GetCurSel(hTab);

    BOOL showMainTabUI = (iPage == 0);
    BOOL showSort = (iPage >= 1 && iPage != 9);
    BOOL showFilter = (iPage >= 1 && iPage <= 7);

    ShowWindow(hTitleLabel, showMainTabUI ? SW_SHOW : SW_HIDE);
    ShowWindow(hDescLabel, showMainTabUI ? SW_SHOW : SW_HIDE);
    ShowWindow(hJarScanButton, showMainTabUI ? SW_SHOW : SW_HIDE);
    ShowWindow(hGlobalClassSearchButton, showMainTabUI ? SW_SHOW : SW_HIDE);
    ShowWindow(hNijikaImage, showMainTabUI ? SW_SHOW : SW_HIDE);
    ShowWindow(hMultiThreadCheckBox, showMainTabUI ? SW_SHOW : SW_HIDE);

    ShowWindow(hSortComboBox, showSort ? SW_SHOW : SW_HIDE);
    ShowWindow(hFilterComboBox, showFilter ? SW_SHOW : SW_HIDE);

    ShowWindow(hSearchMaven, iPage == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(hMavenList, iPage == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchGradle, iPage == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(hGradleList, iPage == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchForge, iPage == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(hForgeList, iPage == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchFabric, iPage == 4 ? SW_SHOW : SW_HIDE);
    ShowWindow(hFabricList, iPage == 4 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchMcp, iPage == 5 ? SW_SHOW : SW_HIDE);
    ShowWindow(hMcpList, iPage == 5 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchLibs, iPage == 6 ? SW_SHOW : SW_HIDE);
    ShowWindow(hLibsList, iPage == 6 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchRunnableJars, iPage == 7 ? SW_SHOW : SW_HIDE);
    ShowWindow(hRunnableJarsList, iPage == 7 ? SW_SHOW : SW_HIDE);
    ShowWindow(hJarButton, iPage == 8 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchClasses, iPage == 8 ? SW_SHOW : SW_HIDE);
    ShowWindow(hJarList, iPage == 8 ? SW_SHOW : SW_HIDE);
    ShowWindow(hEntropyLabel, iPage == 8 ? SW_SHOW : SW_HIDE);
    ShowWindow(hJournalButton, iPage == 9 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchJournal, iPage == 9 ? SW_SHOW : SW_HIDE);
    ShowWindow(hJournalList, iPage == 9 ? SW_SHOW : SW_HIDE);
    ShowWindow(hRecentButton, iPage == 10 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchRecent, iPage == 10 ? SW_SHOW : SW_HIDE);
    ShowWindow(hRecentList, iPage == 10 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPrefetchButton, iPage == 11 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchPrefetch, iPage == 11 ? SW_SHOW : SW_HIDE);
    ShowWindow(hPrefetchList, iPage == 11 ? SW_SHOW : SW_HIDE);
    ShowWindow(hProcessScanButton, iPage == 12 ? SW_SHOW : SW_HIDE);
    ShowWindow(hSearchProcesses, iPage == 12 ? SW_SHOW : SW_HIDE);
    ShowWindow(hProcessList, iPage == 12 ? SW_SHOW : SW_HIDE);
}

void HandleSmoothScroll(WPARAM wParam) {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    int scrollAmount = (delta > 0) ? -40 : 40;

    int iPage = TabCtrl_GetCurSel(hTab);
    HWND hCurrentList = NULL;

    switch (iPage) {
    case 1: hCurrentList = hMavenList; break;
    case 2: hCurrentList = hGradleList; break;
    case 3: hCurrentList = hForgeList; break;
    case 4: hCurrentList = hFabricList; break;
    case 5: hCurrentList = hMcpList; break;
    case 6: hCurrentList = hLibsList; break;
    case 7: hCurrentList = hRunnableJarsList; break;
    case 8: hCurrentList = hJarList; break;
    case 9: hCurrentList = hJournalList; break;
    case 10: hCurrentList = hRecentList; break;
    case 11: hCurrentList = hPrefetchList; break;
    case 12: hCurrentList = hProcessList; break;
    }

    if (hCurrentList) {
        ListView_Scroll(hCurrentList, 0, scrollAmount);
    }
}