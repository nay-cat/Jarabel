#include "../core/app.h"
#include "action_handlers.h"

void __cdecl ProcessJarFile(HWND hListView, const WCHAR* __restrict jarPath) {
    FILE* file = NULL;
    char* centralDirBuffer = NULL;
    char** classNames = NULL;
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    int classCount = 0;
    BOOL eocdFound = FALSE;

    EnterCriticalSection(&g_listLock);
    ClearMasterList(&g_classesMasterList);
    LeaveCriticalSection(&g_listLock);
    ListView_DeleteAllItems(hListView);

    if (_wfopen_s(&file, jarPath, L"rb") != 0 || !file) {
        MessageBoxW(hListView, L"Could not open the selected JAR file.", L"Error", MB_ICONERROR);
        goto cleanup;
    }

    if (_fseeki64(file, 0, SEEK_END) != 0) goto cleanup;
    long long fileSize = _ftelli64(file);
    if (fileSize < sizeof(EOCDRecord)) goto cleanup;

    EOCDRecord eocd = { 0 };
    long long searchPos = fileSize - sizeof(eocd);
    for (int i = 0; searchPos - i >= 0 && i < 65535; ++i) {
        _fseeki64(file, searchPos - i, SEEK_SET);
        uint32_t signature;
        if (fread(&signature, sizeof(signature), 1, file) == 1 && signature == 0x06054b50) {
            _fseeki64(file, searchPos - i, SEEK_SET);
            if (fread(&eocd, sizeof(eocd), 1, file) == 1) {
                eocdFound = TRUE;
                break;
            }
        }
    }

    if (!eocdFound || eocd.dirSize == 0 || (unsigned long long)eocd.dirOffset + eocd.dirSize > (unsigned long long)fileSize) {
        goto cleanup;
    }

    centralDirBuffer = (char*)malloc(eocd.dirSize);
    classNames = (char**)malloc(eocd.totalEntries * sizeof(char*));

    if (!centralDirBuffer || !classNames) goto cleanup;

    _fseeki64(file, eocd.dirOffset, SEEK_SET);
    if (fread(centralDirBuffer, 1, eocd.dirSize, file) != eocd.dirSize) goto cleanup;

    char* p = centralDirBuffer;
    for (UINT i = 0; i < eocd.totalEntries; ++i) {
        if (p + sizeof(CDHeader) > centralDirBuffer + eocd.dirSize) break;
        CDHeader* header = (CDHeader*)p;
        if (header->sig != 0x02014b50) break;

        DWORD entryDiskSize = sizeof(CDHeader) + header->nameLen + header->extraLen + header->cmtLen;
        if (p + entryDiskSize > centralDirBuffer + eocd.dirSize) break;

        if (header->nameLen > 0 && header->nameLen < MAX_PATH) {
            char filename[MAX_PATH] = { 0 };
            __movsb((unsigned char*)filename, (const unsigned char*)p + sizeof(CDHeader), header->nameLen);
            filename[header->nameLen] = '\0';

            if (strstr(filename, ".class")) {
                classNames[classCount] = _strdup(filename);
                FileInfo* pInfo = (FileInfo*)malloc(sizeof(FileInfo));
                if (classNames[classCount] && pInfo) {
                    MultiByteToWideChar(CP_UTF8, 0, filename, -1, pInfo->szFilePath, MAX_PATH);
                    pInfo->liFileSize.QuadPart = header->uncompSize;
                    memset(&pInfo->ftLastAccessTime, 0, sizeof(FILETIME));
                    pInfo->isObfuscated = FALSE;
                    pInfo->entropy = 0.0;

                    EnterCriticalSection(&g_listLock);
                    AddToMasterList(&g_classesMasterList, pInfo);
                    LeaveCriticalSection(&g_listLock);

                    LVITEMW lvi = { 0 };
                    lvi.mask = LVIF_PARAM;
                    lvi.iItem = classCount;
                    lvi.lParam = (LPARAM)pInfo;
                    ListView_InsertItem(hListView, &lvi);

                    classCount++;
                }
                else {
                    if (classNames[classCount]) free(classNames[classCount]);
                    if (pInfo) free(pInfo);
                }
            }
        }
        p += entryDiskSize;
    }

    double entropy = (classCount > 0) ? CalculateAverageEntropy(classNames, classCount) : 0.0;
    WCHAR szEntropy[100];
    swprintf_s(szEntropy, 100, L"Obfuscation Degree: %.2f %s", entropy, (entropy >= 3.1 && entropy <= 3.5) ? L"(High)" : L"");
    SetWindowTextW(hEntropyLabel, szEntropy);

cleanup:
    if (file) fclose(file);
    if (classNames) {
        for (int i = 0; i < classCount; i++) free(classNames[i]);
        free(classNames);
    }
    free(centralDirBuffer);
    // free(localClassList)
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