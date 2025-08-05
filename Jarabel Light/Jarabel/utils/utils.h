#pragma once

#include <windows.h>

// master_list.c
void InitMasterList(MasterList* pList);
void InitMasterLists();
void AddToMasterList(MasterList* pList, void* item);
void ClearMasterList(MasterList* pList);
void FreeMasterLists();
void FilterAndRepopulateListView(HWND hSearch, HWND hList, MasterList* pMasterList, BOOL isUsnJournal, int filterType);
void MergeThreadLocalLists(ThreadLocalJarLists* tlsLists);
void RepopulateAllJarListViewsFromMasterLists();

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


// clipboard.c
void __cdecl CopyToClipboard(HWND hwnd, const WCHAR* __restrict text, size_t text_len_in_chars);

// misc
double __cdecl CalculateAverageEntropy(char** __restrict stringList, int count);
void* __cdecl memmem(const void* __restrict haystack, size_t haystack_len, const void* __restrict needle, size_t needle_len);