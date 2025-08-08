#include "../core/app.h"

void FilterAndRepopulateListView(HWND hSearch, HWND hList, MasterList* pMasterList, bool isUsnJournal, int filterType) {
    WCHAR szSearchTerm[256];
    GetWindowTextW(hSearch, szSearchTerm, 256);

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    EnterCriticalSection(&g_listLock);
    for (int i = 0; i < pMasterList->count; i++) {
        bool searchTermMatch = FALSE;
        bool filterMatch = FALSE;

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

void InitMasterList(MasterList* pList) {
    pList->items = calloc(10, sizeof(void*));
    if (pList->items == NULL) {
        pList->count = 0;
        pList->capacity = 0;
        return;
    }
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
        int newCapacity = pList->capacity > 0 ? pList->capacity * 2 : 10;

        void** newItems = calloc(newCapacity, sizeof(void*));
        if (newItems == NULL) {
            return;
        }

        if (pList->items && pList->count > 0) {
            memcpy(newItems, pList->items, pList->count * sizeof(void*));
            free(pList->items);
        }

        pList->items = newItems;
        pList->capacity = newCapacity;
    }
    pList->items[pList->count++] = item;
}

// this only clears the list's contents
static void ClearMasterListContents(MasterList* pList) {
    if (!pList || !pList->items) return;

    for (int i = 0; i < pList->capacity; i++) {
        if (pList->items[i]) {
            free(pList->items[i]);
            pList->items[i] = NULL;
        }
    }
    pList->count = 0;
}

void DestroyMasterList(MasterList* pList) {
    if (!pList) return;
    ClearMasterListContents(pList); // important to free the contents first
    free(pList->items);             // then we free the container array
    pList->items = NULL;
    pList->count = 0;
    pList->capacity = 0;
}

void FreeMasterLists() {
    EnterCriticalSection(&g_listLock);

    // destroy the lists that OWN their data
    DestroyMasterList(&g_allJarsMasterList);
    DestroyMasterList(&g_classesMasterList);
    DestroyMasterList(&g_journalMasterList);
    DestroyMasterList(&g_recentMasterList);
    DestroyMasterList(&g_prefetchMasterList);
    DestroyMasterList(&g_processesMasterList);
    DestroyMasterList(&g_globalSearchMasterList);

    // for the "view" lists, we only need to free their container arrays,
    // as their content was already freed when g_allJarsMasterList was destroyed
    free(g_mavenMasterList.items);
    free(g_gradleMasterList.items);
    free(g_forgeMasterList.items);
    free(g_fabricMasterList.items);
    free(g_mcpMasterList.items);
    free(g_libsMasterList.items);
    free(g_runnableJarsMasterList.items);

    g_mavenMasterList.items = NULL; g_mavenMasterList.count = 0; g_mavenMasterList.capacity = 0;
    g_gradleMasterList.items = NULL; g_gradleMasterList.count = 0; g_gradleMasterList.capacity = 0;
    g_forgeMasterList.items = NULL; g_forgeMasterList.count = 0; g_forgeMasterList.capacity = 0;
    g_fabricMasterList.items = NULL; g_fabricMasterList.count = 0; g_fabricMasterList.capacity = 0;
    g_mcpMasterList.items = NULL; g_mcpMasterList.count = 0; g_mcpMasterList.capacity = 0;
    g_libsMasterList.items = NULL; g_libsMasterList.count = 0; g_libsMasterList.capacity = 0;
    g_runnableJarsMasterList.items = NULL; g_runnableJarsMasterList.count = 0; g_runnableJarsMasterList.capacity = 0;

    LeaveCriticalSection(&g_listLock);
}

void ClearAllJarLists() {
    ListView_DeleteAllItems(hMavenList); ListView_DeleteAllItems(hGradleList);
    ListView_DeleteAllItems(hForgeList); ListView_DeleteAllItems(hFabricList);
    ListView_DeleteAllItems(hMcpList); ListView_DeleteAllItems(hLibsList);
    ListView_DeleteAllItems(hRunnableJarsList);

    EnterCriticalSection(&g_listLock);

    DestroyMasterList(&g_allJarsMasterList);

    free(g_mavenMasterList.items);
    g_mavenMasterList.items = NULL;
    free(g_gradleMasterList.items);
    g_gradleMasterList.items = NULL;
    free(g_forgeMasterList.items);
    g_forgeMasterList.items = NULL;
    free(g_fabricMasterList.items);
    g_fabricMasterList.items = NULL;
    free(g_mcpMasterList.items);
    g_mcpMasterList.items = NULL;
    free(g_libsMasterList.items);
    g_libsMasterList.items = NULL;
    free(g_runnableJarsMasterList.items);
    g_runnableJarsMasterList.items = NULL;

    InitMasterList(&g_allJarsMasterList);
    InitMasterList(&g_mavenMasterList);
    InitMasterList(&g_gradleMasterList);
    InitMasterList(&g_forgeMasterList);
    InitMasterList(&g_fabricMasterList);
    InitMasterList(&g_mcpMasterList);
    InitMasterList(&g_libsMasterList);
    InitMasterList(&g_runnableJarsMasterList);

    LeaveCriticalSection(&g_listLock);
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