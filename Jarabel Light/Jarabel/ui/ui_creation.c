#include "../core/app.h"

LRESULT CALLBACK TabSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

void CreateUI(HWND hwnd) {
    HMENU hMenuBar = CreateMenu(); HMENU hViewMenu = CreateMenu();
    AppendMenuW(hViewMenu, MF_STRING, ID_MENU_TOGGLE_DARK_MODE, L"Toggle Light/Dark Mode");
    AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
    SetMenu(hwnd, hMenuBar);

    hTab = CreateWindowW(WC_TABCONTROLW, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 900, 700, hwnd, (HMENU)ID_TAB_CONTROL, NULL, NULL);

    SetWindowSubclass(hTab, TabSubclassProc, 2, 0);

    TCITEMW tie = { 0 }; tie.mask = TCIF_TEXT;
    tie.pszText = L"JAR Scanner"; TabCtrl_InsertItem(hTab, 0, &tie);
    tie.pszText = L"Maven"; TabCtrl_InsertItem(hTab, 1, &tie);
    tie.pszText = L"Gradle"; TabCtrl_InsertItem(hTab, 2, &tie);
    tie.pszText = L"Forge"; TabCtrl_InsertItem(hTab, 3, &tie);
    tie.pszText = L"Fabric"; TabCtrl_InsertItem(hTab, 4, &tie);
    tie.pszText = L"MCP"; TabCtrl_InsertItem(hTab, 5, &tie);
    tie.pszText = L"Libs"; TabCtrl_InsertItem(hTab, 6, &tie);
    tie.pszText = L"Runnable"; TabCtrl_InsertItem(hTab, 7, &tie);
    tie.pszText = L"JAR Analyzer"; TabCtrl_InsertItem(hTab, 8, &tie);
    tie.pszText = L"USN Journal"; TabCtrl_InsertItem(hTab, 9, &tie);
    tie.pszText = L"Recents"; TabCtrl_InsertItem(hTab, 10, &tie);
    tie.pszText = L"Prefetch"; TabCtrl_InsertItem(hTab, 11, &tie);
    tie.pszText = L"Processes"; TabCtrl_InsertItem(hTab, 12, &tie);

    DWORD listViewStyle = WS_CHILD | LVS_REPORT | LVS_OWNERDRAWFIXED;
    LV_COLUMNW lvc_path = { 0 }; lvc_path.mask = LVCF_FMT | LVCF_WIDTH; lvc_path.fmt = LVCFMT_LEFT; lvc_path.cx = 840;

    hTitleLabel = CreateWindowW(L"STATIC", L"Jarabel Light", WS_CHILD | SS_CENTER, 0, 0, 10, 10, hwnd, (HMENU)ID_STATIC_TITLE, NULL, NULL);
    SendMessage(hTitleLabel, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

    const WCHAR* descText = L"Jarabel helps you locate all .jar files on a computer.\n\n"
        L"Click 'Scan All Drives' to begin a comprehensive system scan or "
        L"'Search Class Methods' to perform a global search across previously found files.";
    hDescLabel = CreateWindowW(L"STATIC", descText, WS_CHILD | SS_CENTER, 0, 0, 10, 10, hwnd, (HMENU)ID_STATIC_DESC, NULL, NULL);
    SendMessage(hDescLabel, WM_SETFONT, (WPARAM)g_hFont, TRUE);

    hSortComboBox = CreateWindowW(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP, 10, 35, 180, 200, hwnd, (HMENU)ID_SORT_COMBO, NULL, NULL);
    ComboBox_AddString(hSortComboBox, L"Sort: Size (Ascending)");
    ComboBox_AddString(hSortComboBox, L"Sort: Size (Descending)");
    ComboBox_AddString(hSortComboBox, L"Sort: Date (Ascending)");
    ComboBox_AddString(hSortComboBox, L"Sort: Date (Descending)");
    ComboBox_SetCurSel(hSortComboBox, 1);

    hFilterComboBox = CreateWindowW(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_CHILD | WS_TABSTOP, 200, 35, 180, 200, hwnd, (HMENU)ID_FILTER_COMBO, NULL, NULL);
    ComboBox_AddString(hFilterComboBox, L"Filter: Show All");
    ComboBox_AddString(hFilterComboBox, L"Filter: Show Obfuscated");
    ComboBox_SetCurSel(hFilterComboBox, 0);

    hJarScanButton = CreateWindowW(L"BUTTON", L"Scan All Drives", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, (HMENU)ID_BUTTON_JAR_SCAN, NULL, NULL);
    hGlobalClassSearchButton = CreateWindowW(L"BUTTON", L"Search Class Methods", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 10, 10, hwnd, (HMENU)ID_BUTTON_GLOBAL_CLASS_SEARCH, NULL, NULL);
    hNijikaImage = CreateWindowW(L"STATIC", L"", WS_CHILD | SS_OWNERDRAW, 0, 0, 10, 10, hwnd, (HMENU)ID_NIJIKA_IMAGE, NULL, NULL);

    hSearchMaven = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_MAVEN, NULL, NULL);
    hMavenList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_MAVEN, NULL, NULL);
    ListView_InsertColumn(hMavenList, 0, &lvc_path);
    ListView_SetBkColor(hMavenList, g_darkBgColor);
    SubclassListViewHeader(hMavenList);

    hSearchGradle = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_GRADLE, NULL, NULL);
    hGradleList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_GRADLE, NULL, NULL);
    ListView_InsertColumn(hGradleList, 0, &lvc_path);
    ListView_SetBkColor(hGradleList, g_darkBgColor);
    SubclassListViewHeader(hGradleList);

    hSearchForge = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_FORGE, NULL, NULL);
    hForgeList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_FORGE, NULL, NULL);
    ListView_InsertColumn(hForgeList, 0, &lvc_path);
    ListView_SetBkColor(hForgeList, g_darkBgColor);
    SubclassListViewHeader(hForgeList);

    hSearchFabric = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_FABRIC, NULL, NULL);
    hFabricList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_FABRIC, NULL, NULL);
    ListView_InsertColumn(hFabricList, 0, &lvc_path);
    ListView_SetBkColor(hFabricList, g_darkBgColor);
    SubclassListViewHeader(hFabricList);

    hSearchMcp = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_MCP, NULL, NULL);
    hMcpList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_MCP, NULL, NULL);
    ListView_InsertColumn(hMcpList, 0, &lvc_path);
    ListView_SetBkColor(hMcpList, g_darkBgColor);
    SubclassListViewHeader(hMcpList);

    hSearchLibs = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_LIBS, NULL, NULL);
    hLibsList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_LIBS, NULL, NULL);
    ListView_InsertColumn(hLibsList, 0, &lvc_path);
    ListView_SetBkColor(hLibsList, g_darkBgColor);
    SubclassListViewHeader(hLibsList);

    hSearchRunnableJars = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_RUNNABLEJARS, NULL, NULL);
    hRunnableJarsList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 600, hwnd, (HMENU)ID_LISTVIEW_RUNNABLEJARS, NULL, NULL);
    ListView_InsertColumn(hRunnableJarsList, 0, &lvc_path);
    ListView_SetBkColor(hRunnableJarsList, g_darkBgColor);
    SubclassListViewHeader(hRunnableJarsList);

    hJarButton = CreateWindowW(L"BUTTON", L"Select .jar File", WS_CHILD | WS_TABSTOP, 200, 35, 120, 25, hwnd, (HMENU)ID_BUTTON_OPEN, NULL, NULL);
    hSearchClasses = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 325, 35, 340, 25, hwnd, (HMENU)ID_SEARCH_CLASSES, NULL, NULL);
    hEntropyLabel = CreateWindowW(L"STATIC", L"Obfuscation Degree: N/A", WS_CHILD | SS_CENTERIMAGE, 670, 35, 200, 25, hwnd, (HMENU)ID_ENTROPY_LABEL, NULL, NULL);
    hJarList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 65, 860, 575, hwnd, (HMENU)ID_LISTVIEW_CLASSES, NULL, NULL);
    ListView_InsertColumn(hJarList, 0, &lvc_path);
    ListView_SetBkColor(hJarList, g_darkBgColor);
    SubclassListViewHeader(hJarList);

    hJournalButton = CreateWindowW(L"BUTTON", L"Scan All NTFS Drives", WS_CHILD | WS_TABSTOP, 10, 35, 180, 25, hwnd, (HMENU)ID_BUTTON_JOURNAL, NULL, NULL);
    hSearchJournal = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 195, 35, 675, 20, hwnd, (HMENU)ID_SEARCH_JOURNAL, NULL, NULL);
    hJournalList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 580, hwnd, (HMENU)ID_LISTVIEW_JOURNAL, NULL, NULL);
    LV_COLUMNW lvc_journal = { 0 }; lvc_journal.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc_journal.pszText = L"Drive"; lvc_journal.cx = 50; ListView_InsertColumn(hJournalList, 0, &lvc_journal);
    lvc_journal.pszText = L"File Name"; lvc_journal.cx = 350; ListView_InsertColumn(hJournalList, 1, &lvc_journal);
    lvc_journal.pszText = L"Reason"; lvc_journal.cx = 250; ListView_InsertColumn(hJournalList, 2, &lvc_journal);
    lvc_journal.pszText = L"Timestamp (Local)"; lvc_journal.cx = 200; ListView_InsertColumn(hJournalList, 3, &lvc_journal);
    ListView_SetBkColor(hJournalList, g_darkBgColor);
    SubclassListViewHeader(hJournalList);

    hRecentButton = CreateWindowW(L"BUTTON", L"Check Recent Docs", WS_CHILD | WS_TABSTOP, 200, 35, 180, 25, hwnd, (HMENU)ID_BUTTON_RECENT, NULL, NULL);
    hSearchRecent = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_RECENT, NULL, NULL);
    hRecentList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 580, hwnd, (HMENU)ID_LISTVIEW_RECENT, NULL, NULL);
    ListView_InsertColumn(hRecentList, 0, &lvc_path);
    ListView_SetBkColor(hRecentList, g_darkBgColor);
    SubclassListViewHeader(hRecentList);

    hPrefetchButton = CreateWindowW(L"BUTTON", L"Scan Prefetch Folder", WS_CHILD | WS_TABSTOP, 200, 35, 180, 25, hwnd, (HMENU)ID_BUTTON_PREFETCH, NULL, NULL);
    hSearchPrefetch = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 390, 35, 480, 20, hwnd, (HMENU)ID_SEARCH_PREFETCH, NULL, NULL);
    hPrefetchList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 580, hwnd, (HMENU)ID_LISTVIEW_PREFETCH, NULL, NULL);
    ListView_InsertColumn(hPrefetchList, 0, &lvc_path);
    ListView_SetBkColor(hPrefetchList, g_darkBgColor);
    SubclassListViewHeader(hPrefetchList);

    hProcessScanButton = CreateWindowW(L"BUTTON", L"Scan Memory", WS_CHILD | WS_TABSTOP, 200, 35, 220, 25, hwnd, (HMENU)ID_BUTTON_PROCESS_SCAN, NULL, NULL);
    hSearchProcesses = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 430, 35, 440, 20, hwnd, (HMENU)ID_SEARCH_PROCESSES, NULL, NULL);
    hProcessList = CreateWindowExW(0, WC_LISTVIEWW, L"", listViewStyle, 10, 60, 860, 580, hwnd, (HMENU)ID_LISTVIEW_PROCESSES, NULL, NULL);
    ListView_InsertColumn(hProcessList, 0, &lvc_path);
    ListView_SetBkColor(hProcessList, g_darkBgColor);
    SubclassListViewHeader(hProcessList);

    ShowWindow(hTitleLabel, SW_SHOW);
    ShowWindow(hDescLabel, SW_SHOW);
    ShowWindow(hJarScanButton, SW_SHOW);
    ShowWindow(hGlobalClassSearchButton, SW_SHOW);
    ShowWindow(hNijikaImage, SW_SHOW);

    ShowWindow(hSortComboBox, SW_HIDE);
    ShowWindow(hFilterComboBox, SW_HIDE);
    ShowWindow(hSearchMaven, SW_HIDE); ShowWindow(hMavenList, SW_HIDE);
    ShowWindow(hSearchGradle, SW_HIDE); ShowWindow(hGradleList, SW_HIDE);
    ShowWindow(hSearchForge, SW_HIDE); ShowWindow(hForgeList, SW_HIDE);
    ShowWindow(hSearchFabric, SW_HIDE); ShowWindow(hFabricList, SW_HIDE);
    ShowWindow(hSearchMcp, SW_HIDE); ShowWindow(hMcpList, SW_HIDE);
    ShowWindow(hSearchLibs, SW_HIDE); ShowWindow(hLibsList, SW_HIDE);
    ShowWindow(hSearchRunnableJars, SW_HIDE); ShowWindow(hRunnableJarsList, SW_HIDE);
    ShowWindow(hJarButton, SW_HIDE); ShowWindow(hSearchClasses, SW_HIDE); ShowWindow(hJarList, SW_HIDE); ShowWindow(hEntropyLabel, SW_HIDE);
    ShowWindow(hJournalButton, SW_HIDE); ShowWindow(hSearchJournal, SW_HIDE); ShowWindow(hJournalList, SW_HIDE);
    ShowWindow(hRecentButton, SW_HIDE); ShowWindow(hSearchRecent, SW_HIDE); ShowWindow(hRecentList, SW_HIDE);
    ShowWindow(hPrefetchButton, SW_HIDE); ShowWindow(hSearchPrefetch, SW_HIDE); ShowWindow(hPrefetchList, SW_HIDE);
    ShowWindow(hProcessScanButton, SW_HIDE); ShowWindow(hSearchProcesses, SW_HIDE); ShowWindow(hProcessList, SW_HIDE);
}