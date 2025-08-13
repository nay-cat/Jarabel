#pragma once

#include <windows.h>    
#include <commctrl.h>    
#include <windowsx.h>    
#include <stdio.h>       
#include <stdint.h>       
#include <winioctl.h>    
#include <shlwapi.h>     
#include <process.h>    
#include <psapi.h>       
#include <olectl.h>       
#include <dwmapi.h>      
#include <math.h>         
#include <string.h>     
#include <ShlObj.h>      
#include <stdbool.h>

#define ID_ANIMATION_TIMER 1

typedef struct {
    void** items;
    int count;
    int capacity;
} MasterList;

typedef enum {
    JAR_TYPE_NONE = 0,
    JAR_TYPE_MAVEN = 1 << 0,
    JAR_TYPE_GRADLE = 1 << 1,
    JAR_TYPE_FORGE = 1 << 2,
    JAR_TYPE_FABRIC = 1 << 3,
    JAR_TYPE_MCP = 1 << 4,
    JAR_TYPE_LIBS = 1 << 5,
    JAR_TYPE_RUNNABLE = 1 << 6
} JarScanTypeFlags;

typedef struct {
    BYTE type;
    WCHAR szFilePath[MAX_PATH];
    ULARGE_INTEGER liFileSize;
    FILETIME ftLastAccessTime;
    bool isObfuscated;
} FileInfo;

typedef struct {
    WCHAR className[MAX_PATH];
    WCHAR parentJar[MAX_PATH];
} ClassEntry;

typedef struct {
    WCHAR drive[4];
    WCHAR fileName[MAX_PATH];
    WCHAR reason[256];
    WCHAR timestamp[128];
} UsnJournalEntry;

typedef struct {
    HWND hSearchWnd;
    WCHAR searchTerm[256];
} GlobalSearchThreadData;

typedef struct {
    WCHAR jarPath[MAX_PATH];
    int matchCount;
} JarWithMatches;

typedef struct {
    MasterList mavenList;
    MasterList gradleList;
    MasterList forgeList;
    MasterList fabricList;
    MasterList mcpList;
    MasterList libsList;
    MasterList runnableJarsList;
    MasterList allJarsList;
} ThreadLocalJarLists;

#pragma pack(push, 1)

typedef struct { uint32_t sig; uint16_t disk, startDisk, entriesOnDisk, totalEntries; uint32_t dirSize, dirOffset; uint16_t commentLen; } EOCDRecord;
typedef struct { uint32_t sig; uint16_t vMade, vNeeded, flag, comp; uint16_t modTime, modDate; uint32_t crc32, compSize, uncompSize; uint16_t nameLen, extraLen, cmtLen; uint16_t diskStart; uint16_t intAttr; uint32_t extAttr, relOffset; } CDHeader;
typedef struct { uint32_t sig; uint16_t vNeeded, flag, comp; uint16_t modTime, modDate; uint32_t crc32, compSize, uncompSize; uint16_t nameLen, extraLen; } LocalFileHeader;

#pragma pack(pop)

typedef struct {
    HWND hWnd;
    RECT finalRect;
} AnimatedControl;

typedef struct {
    BOOL isAnimating;
    UINT_PTR timerId;
    ULONGLONG startTime;
    int startYOffset;
    int newTab;
    AnimatedControl* controls;
    int numControls;
} TabAnimationState;

extern IPicture* g_pNijikaPicture;
extern HFONT g_hFont, g_hFontBold, g_hFontTitle;
extern HBRUSH g_darkBrush, g_lightBrush, g_darkBtnBrush, g_darkTabBrush, g_tabHighlightBrush;
extern COLORREF g_darkBgColor, g_darkTabColor, g_darkBtnColor, g_darkTextColor, g_whiteColor, g_highlightColor, g_neutralColor;
extern COLORREF g_originalHighlightColor, g_originalHighlightTextColor;
extern BOOL g_isDarkMode;

extern CRITICAL_SECTION g_listLock;

extern HWND hTab;
extern HWND hJarList, hJournalList, hRecentList, hPrefetchList, hProcessList;
extern HWND hMavenList, hGradleList, hForgeList, hFabricList, hMcpList, hLibsList, hRunnableJarsList;
extern HWND hJarButton, hJournalButton, hRecentButton, hPrefetchButton, hJarScanButton, hProcessScanButton, hGlobalClassSearchButton;
extern HWND hSearchMaven, hSearchGradle, hSearchForge, hSearchFabric, hSearchMcp, hSearchLibs, hSearchRunnableJars;
extern HWND hSearchClasses, hSearchJournal, hSearchRecent, hSearchPrefetch, hSearchProcesses;
extern HWND hSortComboBox, hFilterComboBox;
extern HWND hNijikaImage, hEntropyLabel, hTitleLabel, hDescLabel;
extern HWND hTabPageBg;

extern MasterList g_allJarsMasterList;
extern MasterList g_mavenMasterList, g_gradleMasterList, g_forgeMasterList, g_fabricMasterList, g_mcpMasterList, g_libsMasterList, g_runnableJarsMasterList;
extern MasterList g_classesMasterList, g_journalMasterList, g_recentMasterList, g_prefetchMasterList, g_processesMasterList;
extern MasterList g_globalSearchMasterList;

extern int g_clickedItemIndex;
extern HWND g_hClickedListView;

extern int g_currentPage;
extern TabAnimationState g_tabAnimation;