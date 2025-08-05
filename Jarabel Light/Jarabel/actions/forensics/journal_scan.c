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

static BOOL GetUsnFileName(const USN_RECORD_V2* pRecord, WCHAR* buffer, size_t bufferSize) {
    if (!pRecord || !buffer || bufferSize == 0) {
        return FALSE;
    }

    if (pRecord->FileNameOffset >= pRecord->RecordLength ||
        ((DWORD)pRecord->FileNameOffset + pRecord->FileNameLength) > pRecord->RecordLength) {
        return FALSE;
    }

    size_t fileNameChars = pRecord->FileNameLength / sizeof(WCHAR);

    if (fileNameChars >= bufferSize) {
        return FALSE;
    }

    const WCHAR* fileName = (const WCHAR*)((const BYTE*)pRecord + pRecord->FileNameOffset);

    if (memcpy_s(buffer, bufferSize * sizeof(WCHAR), fileName, pRecord->FileNameLength) != 0) {
        return FALSE;
    }

    buffer[fileNameChars] = L'\0';

    return TRUE;
}

static void ProcessUsnRecords(HANDLE hVol, WCHAR* pDrive, int* itemIndex) {
    USN_JOURNAL_DATA_V0 journalData = { 0 };
    DWORD bytesReturned;
    if (!DeviceIoControl(hVol, FSCTL_QUERY_USN_JOURNAL, NULL, 0, &journalData, sizeof(journalData), &bytesReturned, NULL)) return;

    READ_USN_JOURNAL_DATA_V0 readData = { 0 };
    readData.ReasonMask = 0xFFFFFFFF;
    readData.UsnJournalID = journalData.UsnJournalID;

    const DWORD bufferSize = 0x10000;
    BYTE* buffer = (BYTE*)malloc(bufferSize);
    if (!buffer) return;

    while (DeviceIoControl(hVol, FSCTL_READ_USN_JOURNAL, &readData, sizeof(readData), buffer, bufferSize, &bytesReturned, NULL) && bytesReturned > sizeof(USN)) {
        BYTE* pCurrent = buffer + sizeof(USN);
        BYTE* pEnd = buffer + bytesReturned;

        while (pCurrent + sizeof(USN_RECORD_V2) <= pEnd) {
            USN_RECORD_V2* pRecord = (USN_RECORD_V2*)pCurrent;

            if (pRecord->RecordLength == 0 || (pCurrent + pRecord->RecordLength) > pEnd) {
                break;
            }

            WCHAR safeFileName[MAX_PATH];
            if (pRecord->FileNameLength > 0 && GetUsnFileName(pRecord, safeFileName, MAX_PATH)) {
                if (StrStrIW(safeFileName, L".JAR") != NULL) {
                    UsnJournalEntry* pEntry = malloc(sizeof(UsnJournalEntry));
                    if (pEntry) {
                        StringCchPrintfW(pEntry->drive, 4, L"%C:", pDrive[0]);
                        FormatUsnReason(pRecord->Reason, pEntry->reason, sizeof(pEntry->reason) / sizeof(WCHAR));
                        FormatLargeIntTime(pRecord->TimeStamp, pEntry->timestamp, sizeof(pEntry->timestamp) / sizeof(WCHAR));
                        wcscpy_s(pEntry->fileName, MAX_PATH, safeFileName);

                        EnterCriticalSection(&g_listLock);
                        AddToMasterList(&g_journalMasterList, pEntry);
                        LeaveCriticalSection(&g_listLock);

                        AddJournalEntryToView(pEntry, itemIndex);
                    }
                }
            }

            pCurrent += pRecord->RecordLength;
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
    InitializeJournalScan();
    WCHAR driveStrings[MAX_PATH];
    if (GetLogicalDriveStringsW(MAX_PATH, driveStrings) == 0) {
        MessageBoxW(hwnd, L"Failed to get logical drives.", L"Error", MB_ICONERROR);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return;
    }

    WCHAR* pDrive = driveStrings;
    int itemIndex = 0;
    while (*pDrive) {
        size_t remaining_size = MAX_PATH - (pDrive - driveStrings);
        size_t len = wcsnlen_s(pDrive, remaining_size);

        if (len == 0 || len >= remaining_size) {
            break; 
        }

        WCHAR fsNameBuffer[MAX_PATH];
        if (GetVolumeInformationW(pDrive, NULL, 0, NULL, NULL, NULL, fsNameBuffer, MAX_PATH) && wcscmp(fsNameBuffer, L"NTFS") == 0) {
            WCHAR volPath[MAX_PATH];
            BOOL backslash_removed = FALSE;

            if (pDrive[len - 1] == L'\\') {
                pDrive[len - 1] = L'\0';
                backslash_removed = TRUE;
            }

            StringCchPrintfW(volPath, MAX_PATH, L"\\\\.\\%s", pDrive);

            if (backslash_removed) {
                pDrive[len - 1] = L'\\';
            }

            HANDLE hVol = CreateFileW(volPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
            if (hVol != INVALID_HANDLE_VALUE) {
                ProcessUsnRecords(hVol, pDrive, &itemIndex);
                CloseHandle(hVol);
            }
        }
        pDrive += len + 1;
    }
    FinalizeJournalScan(hwnd, itemIndex);
}