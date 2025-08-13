#include "globals.h"

IPicture* g_pNijikaPicture = NULL;
CRITICAL_SECTION g_listLock;

HWND hTab, hJarList, hJournalList, hRecentList, hPrefetchList, hProcessList;
HWND hJarButton, hJournalButton, hRecentButton, hPrefetchButton, hJarScanButton, hProcessScanButton, hGlobalClassSearchButton;
HWND hMavenList, hGradleList, hForgeList, hFabricList, hMcpList, hLibsList, hRunnableJarsList;
HWND hSearchMaven, hSearchGradle, hSearchForge, hSearchFabric, hSearchMcp, hSearchLibs, hSearchRunnableJars;
HWND hSearchClasses, hSearchJournal, hSearchRecent, hSearchPrefetch, hSearchProcesses;
HWND hSortComboBox, hFilterComboBox;
HWND hNijikaImage, hEntropyLabel, hTitleLabel, hDescLabel;

HWND hTabPageBg = NULL;

BOOL g_isDarkMode = TRUE;

HBRUSH g_darkBrush, g_lightBrush, g_darkBtnBrush, g_darkTabBrush, g_tabHighlightBrush;

COLORREF g_darkBgColor = RGB(24, 24, 27);      /* page background - near-black */
COLORREF g_darkTabColor = RGB(34, 36, 42);     /* tab strip / header slightly lighter */
COLORREF g_darkBtnColor = RGB(50, 50, 54);     /* buttons */
COLORREF g_darkTextColor = RGB(230, 230, 235); /* main text: soft white */
COLORREF g_whiteColor = RGB(255, 255, 255);
COLORREF g_highlightColor = RGB(0, 180, 255);  /* accent/teal-blue */
COLORREF g_neutralColor = RGB(160, 160, 165);  /* neutral secondary text */
COLORREF g_originalHighlightColor, g_originalHighlightTextColor;

HFONT g_hFont = NULL, g_hFontBold = NULL, g_hFontTitle = NULL;

MasterList g_allJarsMasterList;
MasterList g_mavenMasterList, g_gradleMasterList, g_forgeMasterList, g_fabricMasterList, g_mcpMasterList, g_libsMasterList, g_runnableJarsMasterList;
MasterList g_classesMasterList, g_journalMasterList, g_recentMasterList, g_prefetchMasterList, g_processesMasterList;
MasterList g_globalSearchMasterList;

int g_clickedItemIndex = -1;
HWND g_hClickedListView = NULL;

int g_currentPage = 0;
TabAnimationState g_tabAnimation = { 0 };