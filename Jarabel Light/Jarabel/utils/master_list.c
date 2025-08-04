#include "../core/app.h"

void InitMasterList(MasterList* pList) {
    pList->items = calloc(10, sizeof(void*));
    if (pList->items == NULL) {
        pList->count = 0;
        pList->capacity = 0;
        return;
    }

    // memset(pList->items, 0, 10 * sizeof(void*));

    pList->count = 0;
    pList->capacity = 10; 
}

void InitMasterLists() {
    InitMasterList(&g_allJarsMasterList);
    InitMasterList(&g_mavenMasterList);
    InitMasterList(&g_gradleMasterList);
    InitMasterList(&g_forgeMasterList);
    InitMasterList(&g_fabricMasterList);
    InitMasterList(&g_mcpMasterList);
    InitMasterList(&g_libsMasterList);
    InitMasterList(&g_runnableJarsMasterList);
    InitMasterList(&g_classesMasterList);
    InitMasterList(&g_journalMasterList);
    InitMasterList(&g_recentMasterList);
    InitMasterList(&g_prefetchMasterList);
    InitMasterList(&g_processesMasterList);
    InitMasterList(&g_globalSearchMasterList);
}

void AddToMasterList(MasterList* pList, void* item) {
    if (pList->count >= pList->capacity) {
        int oldCapacity = pList->capacity;
        int newCapacity = pList->capacity * 2; 
        if (newCapacity == 0) {
            newCapacity = 10; 
        }

        void** newItems = realloc(pList->items, newCapacity * sizeof(void*));
        if (newItems == NULL) {
            return;
        }
        pList->items = newItems;
        pList->capacity = newCapacity;

        if (oldCapacity < newCapacity) {
            memset(pList->items + oldCapacity, 0, ((unsigned long long)(newCapacity) - oldCapacity) * sizeof(void*));
        }
    }
    pList->items[pList->count++] = item;
}

void ClearMasterList(MasterList* pList) {
    if (!pList || !pList->items) return;

    for (int i = 0; i < pList->count; i++) {
        if (pList->items[i]) { // in practice, use of uninit memory is impossible here
            free(pList->items[i]);
        }
    }
    pList->count = 0;
}

void FreeMasterLists() {
    EnterCriticalSection(&g_listLock);

    ClearMasterList(&g_allJarsMasterList);
    free(g_allJarsMasterList.items);

    free(g_mavenMasterList.items);
    free(g_gradleMasterList.items);
    free(g_forgeMasterList.items);
    free(g_fabricMasterList.items);
    free(g_mcpMasterList.items);
    free(g_libsMasterList.items);
    free(g_runnableJarsMasterList.items);

    ClearMasterList(&g_classesMasterList); free(g_classesMasterList.items);
    ClearMasterList(&g_journalMasterList); free(g_journalMasterList.items);
    ClearMasterList(&g_recentMasterList); free(g_recentMasterList.items);
    ClearMasterList(&g_prefetchMasterList); free(g_prefetchMasterList.items);
    ClearMasterList(&g_processesMasterList); free(g_processesMasterList.items);
    ClearMasterList(&g_globalSearchMasterList); free(g_globalSearchMasterList.items);

    LeaveCriticalSection(&g_listLock);
}

void FilterAndRepopulateListView(HWND hSearch, HWND hList, MasterList* pMasterList, BOOL isUsnJournal, int filterType) {
    WCHAR szSearchTerm[256];
    GetWindowTextW(hSearch, szSearchTerm, 256);

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    EnterCriticalSection(&g_listLock);
    for (int i = 0; i < pMasterList->count; i++) {
        BOOL searchTermMatch = FALSE;
        BOOL filterMatch = FALSE;

        if (isUsnJournal) {
            UsnJournalEntry* pEntry = (UsnJournalEntry*)pMasterList->items[i];
            if (szSearchTerm[0] == L'\0' || StrStrIW(pEntry->fileName, szSearchTerm) || StrStrIW(pEntry->reason, szSearchTerm)) {
                searchTermMatch = TRUE;
            }
            filterMatch = TRUE; 
        }
        else {
            FileInfo* pInfo = (FileInfo*)pMasterList->items[i];
            if (!pInfo) continue; 
            if (szSearchTerm[0] == L'\0' || StrStrIW(pInfo->szFilePath, szSearchTerm)) {
                searchTermMatch = TRUE;
            }
            if (filterType == 0 || (filterType == 1 && pInfo->isObfuscated)) {
                filterMatch = TRUE;
            }
        }

        if (searchTermMatch && filterMatch) {
            if (isUsnJournal) {
                UsnJournalEntry* pEntry = (UsnJournalEntry*)pMasterList->items[i];
                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_TEXT;
                lvi.iItem = ListView_GetItemCount(hList);
                lvi.pszText = pEntry->drive;
                ListView_InsertItem(hList, &lvi);
                ListView_SetItemText(hList, lvi.iItem, 1, pEntry->fileName);
                ListView_SetItemText(hList, lvi.iItem, 2, pEntry->reason);
                ListView_SetItemText(hList, lvi.iItem, 3, pEntry->timestamp);
            }
            else {
                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_PARAM;
                lvi.iItem = ListView_GetItemCount(hList);
                lvi.lParam = (LPARAM)pMasterList->items[i];
                ListView_InsertItem(hList, &lvi);
            }
        }
    }
    LeaveCriticalSection(&g_listLock);

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hList, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void MergeThreadLocalLists(ThreadLocalJarLists* tlsLists) {
    for (int i = 0; i < tlsLists->allJarsList.count; i++) {
        AddToMasterList(&g_allJarsMasterList, tlsLists->allJarsList.items[i]);
    }
    for (int i = 0; i < tlsLists->mavenList.count; i++) {
        AddToMasterList(&g_mavenMasterList, tlsLists->mavenList.items[i]);
    }
    for (int i = 0; i < tlsLists->gradleList.count; i++) {
        AddToMasterList(&g_gradleMasterList, tlsLists->gradleList.items[i]);
    }
    for (int i = 0; i < tlsLists->forgeList.count; i++) {
        AddToMasterList(&g_forgeMasterList, tlsLists->forgeList.items[i]);
    }
    for (int i = 0; i < tlsLists->fabricList.count; i++) {
        AddToMasterList(&g_fabricMasterList, tlsLists->fabricList.items[i]);
    }
    for (int i = 0; i < tlsLists->mcpList.count; i++) {
        AddToMasterList(&g_mcpMasterList, tlsLists->mcpList.items[i]);
    }
    for (int i = 0; i < tlsLists->libsList.count; i++) {
        AddToMasterList(&g_libsMasterList, tlsLists->libsList.items[i]);
    }
    for (int i = 0; i < tlsLists->runnableJarsList.count; i++) {
        AddToMasterList(&g_runnableJarsMasterList, tlsLists->runnableJarsList.items[i]);
    }
}

static void RepopulateSingleListView(HWND hList, MasterList* pList) {
    if (!hList || !pList) return;

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    for (int i = 0; i < pList->count; i++) {
        LVITEMW lvi = { 0 };
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i; 
        lvi.lParam = (LPARAM)pList->items[i];
        ListView_InsertItem(hList, &lvi);
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hList, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void RepopulateAllJarListViewsFromMasterLists() {
    RepopulateSingleListView(hMavenList, &g_mavenMasterList);
    RepopulateSingleListView(hGradleList, &g_gradleMasterList);
    RepopulateSingleListView(hForgeList, &g_forgeMasterList);
    RepopulateSingleListView(hFabricList, &g_fabricMasterList);
    RepopulateSingleListView(hMcpList, &g_mcpMasterList);
    RepopulateSingleListView(hLibsList, &g_libsMasterList);
    RepopulateSingleListView(hRunnableJarsList, &g_runnableJarsMasterList);
}