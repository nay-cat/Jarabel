#include "../core/app.h"

void SetControlsForTab(int iPage, BOOL bShow);

void RepopulateActiveList() {
    const int iPage = TabCtrl_GetCurSel(hTab);
    int filterType = ComboBox_GetCurSel(hFilterComboBox);
    bool isUsn = FALSE;

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

void HandleAppResize(const LPARAM lParam) {
    const int width = LOWORD(lParam);
    const int height = HIWORD(lParam);

    MoveWindow(hTab, 0, 0, width, height, TRUE);

    const int topMargin = 35;
    const int controlHeight = 25;
    const int spacing = 5;
    const int listTop = topMargin + controlHeight + spacing;

    const int center_x = width / 2;

    MoveWindow(hTitleLabel, 0, topMargin + 20, width, 70, TRUE);
    MoveWindow(hDescLabel, center_x - 250, topMargin + 100, 500, 80, TRUE);
    MoveWindow(hJarScanButton, center_x - 155, topMargin + 190, 150, 30, TRUE);
    MoveWindow(hGlobalClassSearchButton, center_x + 5, topMargin + 190, 150, 30, TRUE);
    MoveWindow(hNijikaImage, center_x - 125, height - 300, 250, 250, TRUE);

    MoveWindow(hSortComboBox, 10, topMargin, 180, 200, TRUE);
    MoveWindow(hFilterComboBox, 10 + 180 + spacing, topMargin, 180, 200, TRUE);

    const int searchLeft = 10 + 180 + spacing + 180 + spacing;
    const int searchWidth = width - searchLeft - 15;

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

void ToggleDarkMode(const HWND hwnd) {
    g_isDarkMode = !g_isDarkMode;

    SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)(g_isDarkMode ? g_darkBrush : g_lightBrush));

    DwmSetWindowAttribute(hwnd, 20, &g_isDarkMode, sizeof(g_isDarkMode));

    const COLORREF bkColor = g_isDarkMode ? g_darkBgColor : GetSysColor(COLOR_WINDOW);

    const HWND allListViews[] = {
        hMavenList, hGradleList, hForgeList, hFabricList, hMcpList, hLibsList,
        hRunnableJarsList, hJarList, hJournalList, hRecentList, hPrefetchList, hProcessList
    };
    const int numListViews = sizeof(allListViews) / sizeof(allListViews);

    for (int i = 0; i < numListViews; ++i) {
        if (allListViews[i]) {
            ListView_SetBkColor(allListViews[i], bkColor);
            const HWND hHeader = ListView_GetHeader(allListViews[i]);
            if (hHeader) {
                RedrawWindow(hHeader, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
        }
    }

    DrawMenuBar(hwnd);

    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

static AnimatedControl* GetControlsForTab(int iPage, int* pCount) {
    HWND handles[20] = { 0 };
    int count = 0;

    switch (iPage) {
    case 0:
        handles[count++] = hTitleLabel;
        handles[count++] = hDescLabel;
        handles[count++] = hJarScanButton;
        handles[count++] = hGlobalClassSearchButton;
        handles[count++] = hNijikaImage;
        break;
    case 1:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchMaven; handles[count++] = hMavenList;
        break;
    case 2:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchGradle; handles[count++] = hGradleList;
        break;
    case 3:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchForge; handles[count++] = hForgeList;
        break;
    case 4:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchFabric; handles[count++] = hFabricList;
        break;
    case 5:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchMcp; handles[count++] = hMcpList;
        break;
    case 6:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchLibs; handles[count++] = hLibsList;
        break;
    case 7:
        handles[count++] = hSortComboBox; handles[count++] = hFilterComboBox;
        handles[count++] = hSearchRunnableJars; handles[count++] = hRunnableJarsList;
        break;
    case 8:
        handles[count++] = hJarButton; handles[count++] = hSearchClasses;
        handles[count++] = hJarList; handles[count++] = hEntropyLabel;
        break;
    case 9:
        handles[count++] = hJournalButton; handles[count++] = hSearchJournal;
        handles[count++] = hJournalList;
        break;
    case 10:
        handles[count++] = hSortComboBox;
        handles[count++] = hRecentButton; handles[count++] = hSearchRecent;
        handles[count++] = hRecentList;
        break;
    case 11:
        handles[count++] = hSortComboBox;
        handles[count++] = hPrefetchButton; handles[count++] = hSearchPrefetch;
        handles[count++] = hPrefetchList;
        break;
    case 12:
        handles[count++] = hSortComboBox;
        handles[count++] = hProcessScanButton; handles[count++] = hSearchProcesses;
        handles[count++] = hProcessList;
        break;
    }

    *pCount = count;
    if (count == 0) return NULL;

    AnimatedControl* pControls = malloc(count * sizeof(AnimatedControl));
    if (pControls) {
        for (int i = 0; i < count; i++) {
            pControls[i].hWnd = handles[i];
        }
    }
    return pControls;
}

void SetControlsForTab(int iPage, BOOL bShow) {
    int count = 0;
    AnimatedControl* controls = GetControlsForTab(iPage, &count);
    if (controls) {
        for (int i = 0; i < count; i++) {
            ShowWindow(controls[i].hWnd, bShow ? SW_SHOW : SW_HIDE);
        }
        free(controls);
    }
}

void HandleTabChange(HWND hwnd) {
    if (g_tabAnimation.isAnimating) return;

    const int iPage = TabCtrl_GetCurSel(hTab);
    if (iPage == g_currentPage) return;

    SetControlsForTab(g_currentPage, FALSE);

    g_tabAnimation.newTab = iPage;
    g_tabAnimation.controls = GetControlsForTab(iPage, &g_tabAnimation.numControls);
    if (!g_tabAnimation.controls) {
        g_currentPage = iPage;
        return;
    }

    g_tabAnimation.startYOffset = 30;

    HDWP hdwp = BeginDeferWindowPos(g_tabAnimation.numControls);
    if (hdwp) {
        for (int i = 0; i < g_tabAnimation.numControls; i++) {
            GetWindowRect(g_tabAnimation.controls[i].hWnd, &g_tabAnimation.controls[i].finalRect);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&g_tabAnimation.controls[i].finalRect, 2);

            DeferWindowPos(hdwp, g_tabAnimation.controls[i].hWnd, NULL,
                g_tabAnimation.controls[i].finalRect.left,
                g_tabAnimation.controls[i].finalRect.top + g_tabAnimation.startYOffset,
                0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        EndDeferWindowPos(hdwp);
    }

    g_tabAnimation.isAnimating = TRUE;
    g_tabAnimation.startTime = GetTickCount64();
    g_tabAnimation.timerId = SetTimer(hwnd, ID_ANIMATION_TIMER, 10, NULL);

    g_currentPage = iPage;
}

void HandleSmoothScroll(const WPARAM wParam) {
    const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    const int scrollAmount = (delta > 0) ? -40 : 40;

    const int iPage = TabCtrl_GetCurSel(hTab);
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