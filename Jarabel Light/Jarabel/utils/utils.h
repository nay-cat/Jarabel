#include <windows.h>
#include "../core/app.h"

// master_list.c
void InitMasterList(MasterList* pList);
void InitMasterLists();
void AddToMasterList(MasterList* pList, void* item);
void DestroyMasterList(MasterList* pList);
void FreeMasterLists();
void FilterAndRepopulateListView(HWND hSearch, HWND hList, MasterList* pMasterList, bool isUsnJournal, int filterType);
void MergeThreadLocalLists(ThreadLocalJarLists* tlsLists);
void RepopulateAllJarListViewsFromMasterLists();
void ClearAllJarLists();

// sorting.c
int CompareBySizeAsc(const void* __restrict a, const void* __restrict b);
int CompareBySizeDesc(const void* __restrict a, const void* __restrict b);
int CompareByDateAsc(const void* __restrict a, const void* __restrict b);
int CompareByDateDesc(const void* __restrict a, const void* __restrict b);

// formatters.c
void __cdecl FormatLargeIntTime(LARGE_INTEGER li, WCHAR* __restrict buffer, size_t bufferSize);
void __cdecl FormatFileTime(FILETIME ft, WCHAR* __restrict buffer, size_t bufferSize);
void __cdecl FormatUsnReason(DWORD reason, WCHAR* __restrict buffer, size_t bufferSize);
void __cdecl FormatFileSize(ULARGE_INTEGER size, WCHAR* __restrict buffer, size_t bufferSize);
int Utf8ToWide(LPCCH utf8Str, int bytes, LPWSTR wideStr, int wideChars);

// clipboard.c
void __cdecl CopyToClipboard(HWND hwnd, const WCHAR* __restrict text, size_t text_len_in_chars);

// parser.c
bool __fastcall PopulateFileInfo(FileInfo* pInfo);
typedef void (*JarEntryCallback)(const CDHeader* header, const char* filename, FILE* file, void* context);
bool __fastcall ParseJarCentralDirectory(const WCHAR* jarPath, JarEntryCallback callback, void* context);

// misc (for jar analysis)
double __cdecl CalculateAverageEntropy(char** __restrict stringList, int count);
double __cdecl CalculateEntropyFromCounts(const char* __restrict str, size_t len);
double __cdecl CalculateShannonEntropy(const char* __restrict str, size_t len);
void* __cdecl memmem(const void* __restrict haystack, size_t haystack_len, const void* __restrict needle, size_t needle_len);
typedef int errno_t;
errno_t memset_s(void* dest, size_t destsz, int ch, size_t count);
