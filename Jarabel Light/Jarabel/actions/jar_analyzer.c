#include "../core/app.h"
#include "action_handlers.h"

typedef struct {
    HWND hListView;
    MasterList* localClassList;
    int* uiItemCount;
} AnalyzerContext;

static void ProcessListingEntry(const CDHeader* header, const char* filename, FILE* file, void* context) {
    UNREFERENCED_PARAMETER(file);
    if (!strstr(filename, ".class")) {
        return;
    }

    AnalyzerContext* ctx = (AnalyzerContext*)context;

    FileInfo* pInfo = (FileInfo*)malloc(sizeof(FileInfo));
    if (pInfo) {
        memset_s(pInfo, sizeof(FileInfo), 0, sizeof(FileInfo));

        Utf8ToWide(filename, -1, pInfo->szFilePath, MAX_PATH);
        pInfo->liFileSize.QuadPart = header->uncompSize;
        memset_s(&pInfo->ftLastAccessTime, sizeof(FILETIME), 0, sizeof(FILETIME));
        // pInfo->isObfuscated

        EnterCriticalSection(&g_listLock);
        AddToMasterList(&g_classesMasterList, pInfo);
        LeaveCriticalSection(&g_listLock);

        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = (*ctx->uiItemCount)++;
        lvi.lParam = (LPARAM)pInfo;
        ListView_InsertItem(ctx->hListView, &lvi);

        char* nameDup = _strdup(filename);
        if (nameDup) {
            AddToMasterList(ctx->localClassList, nameDup);
        }
        else {
            free(pInfo);
        }
    }
}

void __cdecl ProcessJarFile(HWND hListView, const WCHAR* __restrict jarPath) {
    const HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    EnterCriticalSection(&g_listLock);
    DestroyMasterList(&g_classesMasterList);
    LeaveCriticalSection(&g_listLock);
    ListView_DeleteAllItems(hListView);

    MasterList localClassList;
    InitMasterList(&localClassList);
    int uiItemCount = 0;

    AnalyzerContext context = { 0 };
    context.hListView = hListView;
    context.localClassList = &localClassList;
    context.uiItemCount = &uiItemCount;

    if (!ParseJarCentralDirectory(jarPath, ProcessListingEntry, &context)) {
        MessageBoxW(hListView, L"Could not open or parse the selected JAR file.", L"Error", MB_ICONERROR);
    }

    const double entropy = (localClassList.count > 0) ? CalculateAverageEntropy((char**)localClassList.items, localClassList.count) : 0.0;
    WCHAR szEntropy[100];
    swprintf_s(szEntropy, 100, L"Obfuscation: %.2f %s", entropy, (entropy >= 3.1 && entropy <= 3.5) ? L"(High)" : L"(Low)");
    SetWindowTextW(hEntropyLabel, szEntropy);

    DestroyMasterList(&localClassList);
    SetCursor(hOldCursor);
}

void __cdecl HandleJarInspect(HWND hwnd) {
    WCHAR szFile[MAX_PATH] = { 0 };
    OPENFILENAMEW ofn = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
    ofn.lpstrFilter = L"Java Archives (*.jar)\0*.jar\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) == TRUE) {
        ProcessJarFile(hJarList, ofn.lpstrFile);
    }
}