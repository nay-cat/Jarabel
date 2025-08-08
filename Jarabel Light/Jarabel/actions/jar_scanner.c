#include "../core/app.h"
#include "action_handlers.h"

typedef struct {
    bool isMaven, isGradle, hasForge, hasFabric;
    bool hasMcp, isRunnable, hasNativeHook;
    int mcpClassCheckCount;
    double totalEntropySum;
    int classFileCount;
} CategorizationContext;

_inline static bool StringEndsWith(const char* __restrict str, size_t str_len, const char* __restrict suffix, size_t suffix_len) {
    if (!str || !suffix) return FALSE;
    if (suffix_len > str_len) return FALSE;
    return _memicmp(str + (str_len - suffix_len), suffix, suffix_len) == 0;
}

_inline static bool FindStringInJarEntry(FILE* __restrict file, const CDHeader* __restrict cdHeader, const char** __restrict searchStrings, const size_t* __restrict searchStringLens, int numStrings) {
    if (cdHeader->compSize == 0 || cdHeader->compSize > 10 * 1024 * 1024) return FALSE;
    if (_fseeki64(file, cdHeader->relOffset, SEEK_SET) != 0) return FALSE;
    LocalFileHeader lfHeader;
    if (fread(&lfHeader, sizeof(lfHeader), 1, file) != 1 || lfHeader.sig != 0x04034b50) return FALSE;
    if (_fseeki64(file, (unsigned long long)lfHeader.nameLen + lfHeader.extraLen, SEEK_CUR) != 0) return FALSE;
    char* fileData = (cdHeader->compSize < 1024) ? (char*)_malloca(cdHeader->compSize) : (char*)malloc(cdHeader->compSize);
    if (!fileData) return FALSE;
    bool found = FALSE;
    if (fread(fileData, 1, cdHeader->compSize, file) == cdHeader->compSize) {
        for (int i = 0; i < numStrings; i++) {
            for (size_t j = 0; j <= cdHeader->compSize - searchStringLens[i]; ++j) {
                if (memcmp(fileData + j, searchStrings[i], searchStringLens[i]) == 0) {
                    found = TRUE;
                    break;
                }
            }
            if (found) break;
        }
    }
    if (cdHeader->compSize >= 1024) free(fileData);
    else _freea(fileData);
    return found;
}

static void ProcessCategorizationEntry(const CDHeader* header, const char* filename, FILE* file, void* context) {
    CategorizationContext* ctx = (CategorizationContext*)context;
    size_t nameLen = strnlen_s(filename, MAX_PATH);

    if (strstr(filename, "pom.xml")) ctx->isMaven = TRUE;
    if (strstr(filename, "build.gradle")) ctx->isGradle = TRUE;
    if (strcmp(filename, "fabric.mod.json") == 0) ctx->hasFabric = TRUE;
    if (strcmp(filename, "mcmod.info") == 0 || strstr(filename, "net/minecraftforge/fml")) ctx->hasForge = TRUE;
    if (strcmp(filename, "META-INF/MANIFEST.MF") == 0) {
        const char* searchStr[] = { "Main-Class:" };
        const size_t searchStrLens[] = { sizeof("Main-Class:") - 1 };
        if (FindStringInJarEntry(file, header, searchStr, searchStrLens, 1)) {
            ctx->isRunnable = TRUE;
        }
    }
    if (StringEndsWith(filename, nameLen, ".class", sizeof(".class") - 1)) {
        ctx->totalEntropySum += CalculateEntropyFromCounts(filename, nameLen);
        ctx->classFileCount++;

        if (!ctx->hasMcp && ctx->mcpClassCheckCount++ < 10) {
            const char* mcpStrings[] = { "func_", "field_", " mc" };
            const size_t mcpStringsLens[] = { sizeof("func_") - 1, sizeof("field_") - 1, sizeof(" mc") - 1 };
            if (FindStringInJarEntry(file, header, mcpStrings, mcpStringsLens, 3)) {
                ctx->hasMcp = TRUE;
            }
        }
        if (!ctx->hasNativeHook) {
            if (strstr(filename, "org/jnativehook/")) {
                ctx->hasNativeHook = TRUE;
            }
        }
    }
}

void AnalyzeAndCategorizeJar(FileInfo* __restrict pOriginalInfo, HWND hOwnerWnd, ThreadLocalJarLists* __restrict tlsLists) {
    if (!PopulateFileInfo(pOriginalInfo) || pOriginalInfo->liFileSize.QuadPart == (ULONGLONG)-1) {
        free(pOriginalInfo);
        return;
    }

    CategorizationContext context = { 0 };

    ParseJarCentralDirectory(pOriginalInfo->szFilePath, ProcessCategorizationEntry, &context);

    const double averageEntropy = (context.classFileCount > 0)
        ? (context.totalEntropySum / context.classFileCount)
        : 0.0;

    pOriginalInfo->isObfuscated = (pOriginalInfo->liFileSize.QuadPart < 16777216 && averageEntropy >= 3.1 && averageEntropy <= 3.5);

    BYTE types = JAR_TYPE_NONE;
    int categorizedCount = 0;
    if (context.isMaven) { types |= JAR_TYPE_MAVEN; categorizedCount++; }
    if (context.isGradle) { types |= JAR_TYPE_GRADLE; categorizedCount++; }
    if (context.hasForge) { types |= JAR_TYPE_FORGE; categorizedCount++; }
    if (context.hasFabric) { types |= JAR_TYPE_FABRIC; categorizedCount++; }
    if (context.hasMcp) { types |= JAR_TYPE_MCP; categorizedCount++; }
    if (context.isRunnable) { types |= JAR_TYPE_RUNNABLE; categorizedCount++; }
    if (context.hasNativeHook || categorizedCount == 0) {
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