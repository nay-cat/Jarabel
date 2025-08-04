#include "scans.h"

static void InitializeJournalScan() {
    EnterCriticalSection(&g_listLock);
    ClearMasterList(&g_journalMasterList);
    LeaveCriticalSection(&g_listLock);
    ListView_DeleteAllItems(hJournalList);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
}

static void AddJournalEntryToView(UsnJournalEntry* pEntry, int* itemIndex) {
    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = (*itemIndex)++;
    lvi.pszText = pEntry->drive;
    ListView_InsertItem(hJournalList, &lvi);
    ListView_SetItemText(hJournalList, *itemIndex - 1, 1, pEntry->fileName);
    ListView_SetItemText(hJournalList, *itemIndex - 1, 2, pEntry->reason);
    ListView_SetItemText(hJournalList, *itemIndex - 1, 3, pEntry->timestamp);
}

static void ProcessUsnRecords(HANDLE hVol, WCHAR* pDrive, int* itemIndex) {
    USN_JOURNAL_DATA_V0 journalData = { 0 };
    DWORD bytesReturned;
    if (!DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &journalData, sizeof(journalData), &bytesReturned, NULL)) return;

    READ_USN_JOURNAL_DATA_V0 readData = { 0 };
    readData.ReasonMask = 0xFFFFFFFF;
    readData.UsnJournalID = journalData.UsnJournalID;
    BYTE* buffer = (BYTE*)malloc(0x10000);
    if (!buffer) return;

    while (DeviceIoControl(hVol, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData), buffer, 0x10000, &bytesReturned, NULL) && bytesReturned > sizeof(USN)) {
        USN_RECORD_V2* pRecord = (USN_RECORD_V2*)&buffer[sizeof(USN)];
        DWORD recordSize = bytesReturned - sizeof(USN);
        while (recordSize > 0) {
            if (pRecord->FileNameLength > 0 && StrStrIW(pRecord->FileName, L".JAR") != NULL) {
                UsnJournalEntry* pEntry = malloc(sizeof(UsnJournalEntry));
                if (pEntry) {
                    swprintf_s(pEntry->drive, 4, L"%C:", pDrive[0]);
                    FormatUsnReason(pRecord->Reason, pEntry->reason, sizeof(pEntry->reason));
                    FormatLargeIntTime(pRecord->TimeStamp, pEntry->timestamp, sizeof(pEntry->timestamp));
                    wcsncpy_s(pEntry->fileName, MAX_PATH, pRecord->FileName, pRecord->FileNameLength / sizeof(WCHAR));
                    EnterCriticalSection(&g_listLock);
                    AddToMasterList(&g_journalMasterList, pEntry);
                    LeaveCriticalSection(&g_listLock);
                    AddJournalEntryToView(pEntry, itemIndex);
                }
            }
            if (pRecord->RecordLength == 0) break;
            recordSize -= pRecord->RecordLength;
            pRecord = (USN_RECORD_V2*)((PBYTE)pRecord + pRecord->RecordLength);
        }
        readData.StartUsn = *(USN*)&buffer[0];
    }
    free(buffer);
}

static void FinalizeJournalScan(HWND hwnd, int itemIndex) {
    if (itemIndex == 0) {
        MessageBoxW(hwnd, L"No .jar entries were found in the USN Journals of accessible NTFS drives. Ensure that Jarabel is running as admin", L"Scan Finished", MB_ICONINFORMATION);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void HandleJournalCheck(HWND hwnd) {
    InitializeJournalScan(hwnd, hJournalList);
    WCHAR driveStrings[MAX_PATH];
    if (GetLogicalDriveStringsW(MAX_PATH, driveStrings) == 0) {
        MessageBoxW(hwnd, L"Failed to get logical drives.", L"Error", MB_ICONERROR);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    WCHAR* pDrive = driveStrings;
    int itemIndex = 0;
    while (*pDrive) {
        WCHAR fsNameBuffer[MAX_PATH];
        if (GetVolumeInformationW(pDrive, NULL, 0, NULL, NULL, NULL, fsNameBuffer, MAX_PATH) && wcscmp(fsNameBuffer, L"NTFS") == 0) {
            WCHAR volPath[MAX_PATH];
            swprintf_s(volPath, MAX_PATH, L"\\\\.\\%c:", pDrive[0]);
            HANDLE hVol = CreateFileW(volPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hVol != INVALID_HANDLE_VALUE) {
                ProcessUsnRecords(hVol, pDrive, &itemIndex);
                CloseHandle(hVol);
            }
        }
        pDrive += wcslen(pDrive) + 1;
    }
    FinalizeJournalScan(hwnd, itemIndex);
}