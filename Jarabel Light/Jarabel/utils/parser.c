#include "utils.h"

bool __fastcall PopulateFileInfo(FileInfo* pInfo) {
    if (!pInfo || pInfo->szFilePath[0] == L'\0') {
        return FALSE;
    }

    WIN32_FILE_ATTRIBUTE_DATA fad = { 0 };
    if (GetFileAttributesExW(pInfo->szFilePath, GetFileExInfoStandard, &fad)) {
        pInfo->liFileSize.LowPart = fad.nFileSizeLow;
        pInfo->liFileSize.HighPart = fad.nFileSizeHigh;
        pInfo->ftLastAccessTime = fad.ftLastAccessTime;
        return TRUE;
    }

    pInfo->liFileSize.QuadPart = (ULONGLONG)-1;
    return FALSE;
}

bool __fastcall ParseJarCentralDirectory(const WCHAR* jarPath, const JarEntryCallback callback, void* context) {
    FILE* file = NULL;
    char* centralDirBuffer = NULL;
    bool result = FALSE;
    bool eocdFound = FALSE;
    EOCDRecord eocd = { 0 };

    if (_wfopen_s(&file, jarPath, L"rb") != 0 || !file) {
        return FALSE;
    }

    if (_fseeki64(file, 0, SEEK_END) != 0) {
        fclose(file);
        return FALSE;
    }

    const long long fileSize = _ftelli64(file);
    if (fileSize < (long long)sizeof(EOCDRecord)) {
        fclose(file);
        return FALSE;
    }

    const  long long searchPos = fileSize - sizeof(eocd);
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

    centralDirBuffer = (char*)malloc(eocd.dirSize); // could be streamed directly from memory
    if (!centralDirBuffer) {
        goto cleanup;
    }

    if (_fseeki64(file, eocd.dirOffset, SEEK_SET) != 0 || fread(centralDirBuffer, 1, eocd.dirSize, file) != eocd.dirSize) {
        goto cleanup;
    }

    char* p = centralDirBuffer;
    for (UINT i = 0; i < eocd.totalEntries; ++i) {
        if (p + sizeof(CDHeader) > centralDirBuffer + eocd.dirSize) break;
        CDHeader* header = (CDHeader*)p;
        if (header->sig != 0x02014b50) break;

        const DWORD entryDiskSize = sizeof(CDHeader) + header->nameLen + header->extraLen + header->cmtLen;
        if (p + entryDiskSize > centralDirBuffer + eocd.dirSize) break;

        if (header->nameLen > 0 && header->nameLen < MAX_PATH) {
            char filename[MAX_PATH];
            memcpy(filename, (const char*)p + sizeof(CDHeader), header->nameLen);
            filename[header->nameLen] = '\0';

            callback(header, filename, file, context);
        }
        p += entryDiskSize;
    }

    result = TRUE;

cleanup:
    free(centralDirBuffer);
    if (file) {
        fclose(file);
    }
    return result;
}