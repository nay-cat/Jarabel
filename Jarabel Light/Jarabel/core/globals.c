#include "globals.h"

IPicture* g_pNijikaPicture = NULL;
CRITICAL_SECTION g_listLock;

HWND hTab, hJarList, hJournalList, hRecentList, hPrefetchList, hProcessList;
HWND hJarButton, hJournalButton, hRecentButton, hPrefetchButton, hJarScanButton, hProcessScanButton, hGlobalClassSearchButton;
HWND hMavenList, hGradleList, hForgeList, hFabricList, hMcpList, hLibsList, hRunnableJarsList;
HWND hSearchMaven, hSearchGradle, hSearchForge, hSearchFabric, hSearchMcp, hSearchLibs, hSearchRunnableJars;
HWND hSearchClasses, hSearchJournal, hSearchRecent, hSearchPrefetch, hSearchProcesses;
HWND hSortComboBox, hFilterComboBox;
HWND hNijikaImage, hEntropyLabel, hTitleLabel, hDescLabel, hMultiThreadCheckBox;

BOOL g_isDarkMode = TRUE;
BOOL g_useMultiThreading = FALSE; 

HBRUSH g_darkBrush, g_lightBrush, g_darkBtnBrush, g_darkTabBrush, g_tabHighlightBrush;

COLORREF g_darkBgColor = RGB(30, 30, 30);
COLORREF g_darkTextColor = RGB(220, 220, 220);
COLORREF g_whiteColor = RGB(255, 255, 255);
COLORREF g_highlightColor = RGB(0, 255, 127);
COLORREF g_neutralColor = RGB(160, 160, 160);
COLORREF g_originalHighlightColor, g_originalHighlightTextColor;

HFONT g_hFont = NULL, g_hFontBold = NULL, g_hFontTitle = NULL;

MasterList g_allJarsMasterList;
MasterList g_mavenMasterList, g_gradleMasterList, g_forgeMasterList, g_fabricMasterList, g_mcpMasterList, g_libsMasterList, g_runnableJarsMasterList;
MasterList g_classesMasterList, g_journalMasterList, g_recentMasterList, g_prefetchMasterList, g_processesMasterList;
MasterList g_globalSearchMasterList;

int g_clickedItemIndex = -1;
HWND g_hClickedListView = NULL;