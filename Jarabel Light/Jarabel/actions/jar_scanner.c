#include "../core/app.h"
#include "action_handlers.h"

// __restrict to indicate that str and suffix do not overlap in memory
_inline static BOOL StringEndsWith(const char* __restrict str, const char* __restrict suffix) {
    if (!str || !suffix) {
        return FALSE;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) {
        return FALSE;
    }
    return _strcmpi(str + (str_len - suffix_len), suffix) == 0;
}

_inline static BOOL FindStringInJarEntry(FILE* __restrict file, const CDHeader* __restrict cdHeader, const char** __restrict searchStrings, int numStrings) {
    if (cdHeader->compSize == 0 || cdHeader->compSize > 10 * 1024 * 1024) {
        return FALSE;
    }

    if (_fseeki64(file, cdHeader->relOffset, SEEK_SET) != 0) {
        return FALSE;
    }

    LocalFileHeader lfHeader;
    if (fread(&lfHeader, sizeof(lfHeader), 1, file) != 1 || lfHeader.sig != 0x04034b50) {
        return FALSE;
    }

    if (_fseeki64(file, (unsigned long long)lfHeader.nameLen + lfHeader.extraLen, SEEK_CUR) != 0) {
        return FALSE;
    }

    // to keep them on the stack
    char* fileData = (cdHeader->compSize < 1024) ? (char*)_malloca(cdHeader->compSize) : (char*)malloc(cdHeader->compSize);
    if (!fileData) {
        return FALSE;
    }

    if (fread(fileData, 1, cdHeader->compSize, file) != cdHeader->compSize) {
        if (cdHeader->compSize >= 1024) free(fileData);
        else _freea(fileData);
        return FALSE;
    }

    BOOL found = FALSE;
    for (int i = 0; i < numStrings; i++) {
        if (memmem(fileData, cdHeader->compSize, searchStrings[i], strlen(searchStrings[i]))) {
            found = TRUE;
            break;
        }
    }

    if (cdHeader->compSize >= 1024) free(fileData);
    else _freea(fileData);
    return found;
}

void AnalyzeAndCategorizeJar(FileInfo* __restrict pOriginalInfo, HWND hOwnerWnd, ThreadLocalJarLists* __restrict tlsLists) {
    WIN32_FILE_ATTRIBUTE_DATA fad = { 0 };
    if (!GetFileAttributesExW(pOriginalInfo->szFilePath, GetFileExInfoStandard, &fad)) {
        free(pOriginalInfo);
        return;
    }

    pOriginalInfo->liFileSize.LowPart = fad.nFileSizeLow;
    pOriginalInfo->liFileSize.HighPart = fad.nFileSizeHigh;
    pOriginalInfo->ftLastAccessTime = fad.ftLastAccessTime;

    FILE* file;
    if (_wfopen_s(&file, pOriginalInfo->szFilePath, L"rb") != 0 || !file) {
        free(pOriginalInfo);
        return;
    }

    if (_fseeki64(file, 0, SEEK_END) != 0) {
        fclose(file); free(pOriginalInfo); return;
    }
    long long fileSize = _ftelli64(file);
    if (fileSize < sizeof(EOCDRecord)) {
        fclose(file); free(pOriginalInfo); return;
    }

    EOCDRecord eocd = { 0 };
    long long searchPos = fileSize - sizeof(eocd);
    BOOL foundEOCD = FALSE;
    for (int i = 0; searchPos - i >= 0 && i < 65535; ++i) {
        _fseeki64(file, searchPos - i, SEEK_SET);
        uint32_t signature;
        if (fread(&signature, sizeof(signature), 1, file) == 1 && signature == 0x06054b50) {
            _fseeki64(file, searchPos - i, SEEK_SET);
            if (fread(&eocd, sizeof(eocd), 1, file) == 1) { foundEOCD = TRUE; break; }
        }
    }

    if (!foundEOCD || eocd.dirSize == 0 || (unsigned long long)eocd.dirOffset + eocd.dirSize > (unsigned long long)fileSize) {
        fclose(file); free(pOriginalInfo); return;
    }

    char* centralDirBuffer = (char*)malloc(eocd.dirSize);
    if (!centralDirBuffer) {
        fclose(file); free(pOriginalInfo); return;
    }

    _fseeki64(file, eocd.dirOffset, SEEK_SET);
    fread(centralDirBuffer, 1, eocd.dirSize, file);

    BOOL isMaven = FALSE, isGradle = FALSE, hasForge = FALSE, hasFabric = FALSE;
    BOOL hasMcp = FALSE, isRunnable = FALSE, hasNativeHook = FALSE;
    int mcpClassCheckCount = 0, classCount = 0;
    char** classNames = (char**)malloc(eocd.totalEntries * sizeof(char*));

    char* __restrict p = centralDirBuffer;
    for (UINT i = 0; i < eocd.totalEntries; ++i) {
        if (p + sizeof(CDHeader) > centralDirBuffer + eocd.dirSize) break;
        CDHeader* header = (CDHeader*)p;
        if (header->sig != 0x02014b50) break;

        DWORD entryDiskSize = sizeof(CDHeader) + header->nameLen + header->extraLen + header->cmtLen;
        if (p + entryDiskSize > centralDirBuffer + eocd.dirSize) break;

        if (header->nameLen < MAX_PATH) {
            char filename[MAX_PATH] = { 0 };
            __movsb((unsigned char*)filename, (const unsigned char*)p + sizeof(CDHeader), header->nameLen);
            filename[header->nameLen] = '\0';

            if (strstr(filename, "pom.xml")) isMaven = TRUE;
            if (strstr(filename, "build.gradle")) isGradle = TRUE;
            if (strcmp(filename, "fabric.mod.json") == 0) hasFabric = TRUE;
            if (strcmp(filename, "mcmod.info") == 0 || strstr(filename, "net/minecraftforge/fml")) hasForge = TRUE;
            if (strcmp(filename, "META-INF/MANIFEST.MF") == 0) {
                const char* searchStr[] = { "Main-Class:" };
                if (FindStringInJarEntry(file, header, searchStr, 1)) isRunnable = TRUE;
            }
            if (StringEndsWith(filename, ".class")) {
                if (classNames) classNames[classCount++] = _strdup(filename);
                if (!hasMcp && mcpClassCheckCount++ < 10) {
                    const char* mcpStrings[] = { "func_", "field_", "mc" };
                    if (FindStringInJarEntry(file, header, mcpStrings, 3)) hasMcp = TRUE;
                }
                if (!hasNativeHook) {
                    const char* jnativeHookStrings[] = { "org/jnativehook/mouse/" };
                    if (strstr(filename, jnativeHookStrings[0])) hasNativeHook = TRUE;
                }
            }
        }
        p += entryDiskSize;
    }

    pOriginalInfo->entropy = (classCount > 0) ? CalculateAverageEntropy(classNames, classCount) : 0.0;
    pOriginalInfo->isObfuscated = (pOriginalInfo->liFileSize.QuadPart > 16777216 && pOriginalInfo->entropy >= 3.1 && pOriginalInfo->entropy <= 3.5);

    for (int i = 0; i < classCount; i++) free(classNames[i]);
    free(classNames);
    fclose(file);
    free(centralDirBuffer);

    DWORD types = JAR_TYPE_NONE;
    int categorizedCount = 0;
    if (isMaven) { types |= JAR_TYPE_MAVEN; categorizedCount++; }
    if (isGradle) { types |= JAR_TYPE_GRADLE; categorizedCount++; }
    if (hasForge) { types |= JAR_TYPE_FORGE; categorizedCount++; }
    if (hasFabric) { types |= JAR_TYPE_FABRIC; categorizedCount++; }
    if (hasMcp) { types |= JAR_TYPE_MCP; categorizedCount++; }
    if (isRunnable) { types |= JAR_TYPE_RUNNABLE; categorizedCount++; }
    if (hasNativeHook || categorizedCount == 0) {
        types |= JAR_TYPE_LIBS;
    }

    if (types != JAR_TYPE_NONE) {
        pOriginalInfo->type = types;

        if (tlsLists) {
            AddToMasterList(&tlsLists->allJarsList, pOriginalInfo);
            if (types & JAR_TYPE_MAVEN) AddToMasterList(&tlsLists->mavenList, pOriginalInfo);
            if (types & JAR_TYPE_GRADLE) AddToMasterList(&tlsLists->gradleList, pOriginalInfo);
            if (types & JAR_TYPE_FORGE) AddToMasterList(&tlsLists->forgeList, pOriginalInfo);
            if (types & JAR_TYPE_FABRIC) AddToMasterList(&tlsLists->fabricList, pOriginalInfo);
            if (types & JAR_TYPE_MCP) AddToMasterList(&tlsLists->mcpList, pOriginalInfo);
            if (types & JAR_TYPE_RUNNABLE) AddToMasterList(&tlsLists->runnableJarsList, pOriginalInfo);
            if (types & JAR_TYPE_LIBS) AddToMasterList(&tlsLists->libsList, pOriginalInfo);
        }
        else {
            PostMessage(hOwnerWnd, WM_APP_ADD_JAR_SCAN_ITEM, 0, (LPARAM)pOriginalInfo);
        }
    }
    else {
        free(pOriginalInfo);
    }
}