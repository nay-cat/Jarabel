#include "scan.h"

typedef struct DirNode {
    wchar_t* path;         
    struct DirNode* next;
} DirNode;

static CRITICAL_SECTION g_qLock;
static CONDITION_VARIABLE g_qCond;
static CONDITION_VARIABLE g_doneCond;
static DirNode* g_qHead = NULL;
static DirNode* g_qTail = NULL;
static size_t g_qCount = 0;
static size_t g_activeProcessing = 0; 

static FOUND_JAR_CALLBACK g_callback = NULL;
static void* g_callback_context = NULL;

static void push_dir(wchar_t* path) {
    DirNode* n = (DirNode*)malloc(sizeof(DirNode));
    if (!n) { free(path); return; }
    n->path = path;
    n->next = NULL;
    EnterCriticalSection(&g_qLock);
    if (g_qTail) g_qTail->next = n; else g_qHead = n;
    g_qTail = n;
    g_qCount++;
    LeaveCriticalSection(&g_qLock);
    WakeConditionVariable(&g_qCond);
}

static wchar_t* pop_dir_or_wait(void) {
    EnterCriticalSection(&g_qLock);
    for (;;) {
        if (g_qHead != NULL) {
            DirNode* n = g_qHead;
            g_qHead = n->next;
            if (!g_qHead) g_qTail = NULL;
            g_qCount--;
            g_activeProcessing++;
            LeaveCriticalSection(&g_qLock);
            wchar_t* path = n->path;
            free(n);
            return path;
        }
        if (g_activeProcessing == 0) {
            LeaveCriticalSection(&g_qLock);
            return NULL;
        }
        SleepConditionVariableCS(&g_qCond, &g_qLock, INFINITE);
    }
}

static void worker_done_dir(void) {
    EnterCriticalSection(&g_qLock);
    if (g_activeProcessing > 0) g_activeProcessing--;
    if (g_qCount == 0 && g_activeProcessing == 0) {
        WakeConditionVariable(&g_doneCond);
    }
    LeaveCriticalSection(&g_qLock);
}

DWORD WINAPI worker_proc(LPVOID lpParam) {
    (void)lpParam;

    // to avoid stack overflow, otherwise function would use 131708 of stack
    size_t buf_size = 32767;
    wchar_t* pathBuffer = (wchar_t*)malloc(buf_size * sizeof(wchar_t));
    wchar_t* searchPattern = (wchar_t*)malloc(buf_size * sizeof(wchar_t));

    if (!pathBuffer || !searchPattern) {
        fwprintf(stderr, L"worker_proc: failed to allocate buffers.\n");
        free(pathBuffer);
        free(searchPattern);
        return 1;
    }

    while (1) {
        wchar_t* dirPath = pop_dir_or_wait();
        if (!dirPath) break;

        HRESULT hr = StringCchPrintfW(searchPattern, buf_size, L"\\\\?\\%s\\*", dirPath);
        if (FAILED(hr)) {
            worker_done_dir();
            free(dirPath);
            continue;
        }

        WIN32_FIND_DATAW fd = { 0 };
        HANDLE hFind = FindFirstFileExW(searchPattern, FindExInfoBasic, &fd, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (fd.cFileName[0] == L'.' && (fd.cFileName[1] == L'\0' || (fd.cFileName[1] == L'.' && fd.cFileName[2] == L'\0'))) {
                    continue;
                }

                StringCchCopyW(pathBuffer, buf_size, dirPath);
                PathCchAppend(pathBuffer, buf_size, fd.cFileName);

                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

                    wchar_t* qpath = _wcsdup(pathBuffer);
                    if (qpath) {
                        push_dir(qpath);
                    }
                }
                else {
                    size_t nameLen = wcslen(fd.cFileName);
                    if (nameLen > 4 && _wcsicmp(fd.cFileName + nameLen - 4, L".jar") == 0) {
                        if (g_callback) g_callback(pathBuffer, g_callback_context);
                    }
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
        worker_done_dir();
        free(dirPath);
    }

    free(pathBuffer);
    free(searchPattern);
    return 0;
}

void ScanSystemJars(FOUND_JAR_CALLBACK callback, void* context) {
    InitializeCriticalSection(&g_qLock);
    InitializeConditionVariable(&g_qCond);
    InitializeConditionVariable(&g_doneCond);

    g_callback = callback;
    g_callback_context = context;

    wchar_t drives[256];
    if (GetLogicalDriveStringsW(_countof(drives), drives) == 0) {
        DeleteCriticalSection(&g_qLock);
        return;
    }
    wchar_t* p = drives;
    while (*p) {
        if (GetDriveTypeW(p) == DRIVE_FIXED) {
            size_t len = wcslen(p);
            if (len > 0 && p[len - 1] == L'\\') {
                p[len - 1] = L'\0';
            }
            wchar_t* root = _wcsdup(p);
            if (root) {
                push_dir(root);
            }
        }
        p += wcslen(p) + 1;
    }

    SYSTEM_INFO si; GetSystemInfo(&si);
    int cpu = (int)si.dwNumberOfProcessors;
    int threadCount = cpu * THREADS_PER_CPU;
    if (threadCount < 1) threadCount = 1;

    HANDLE* threads = (HANDLE*)calloc((size_t)threadCount, sizeof(HANDLE));
    if (!threads) {
        EnterCriticalSection(&g_qLock);
        while (g_qHead) {
            DirNode* n = g_qHead;
            g_qHead = n->next;
            if (n->path) free(n->path);
            free(n);
        }
        LeaveCriticalSection(&g_qLock);
        DeleteCriticalSection(&g_qLock);
        return;
    }

    for (int i = 0; i < threadCount; ++i) {
        threads[i] = CreateThread(NULL, 0, worker_proc, NULL, 0, NULL);
        if (!threads[i]) {
            fwprintf(stderr, L"CreateThread failed for worker %d (err %lu)\n", i, GetLastError());
        }
    }

    EnterCriticalSection(&g_qLock);
    while (g_qHead != NULL || g_activeProcessing > 0) {
        SleepConditionVariableCS(&g_doneCond, &g_qLock, INFINITE);
    }
    LeaveCriticalSection(&g_qLock);

    WakeAllConditionVariable(&g_qCond);

    for (int i = 0; i < threadCount; ++i) {
        if (threads[i]) { 
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
        }
    }
    free(threads);

    EnterCriticalSection(&g_qLock);
    while (g_qHead) {
        DirNode* n = g_qHead;
        g_qHead = n->next;
        if (n->path) free(n->path);
        free(n);
    }
    g_qTail = NULL;
    LeaveCriticalSection(&g_qLock);

    DeleteCriticalSection(&g_qLock);
}