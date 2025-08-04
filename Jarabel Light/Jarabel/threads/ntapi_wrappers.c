#include "ntapi_wrappers.h"

pNtQueryDirectoryFileEx g_NtQueryDirectoryFileEx = NULL;
pNtOpenFile g_NtOpenFile = NULL;
pRtlInitUnicodeString g_RtlInitUnicodeString = NULL;
pNtClose g_NtClose = NULL;

BOOL InitializeNtApi() {
    if (g_NtQueryDirectoryFileEx) return TRUE;
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return FALSE;
    g_NtQueryDirectoryFileEx = (pNtQueryDirectoryFileEx)GetProcAddress(hNtdll, "NtQueryDirectoryFileEx");
    g_NtOpenFile = (pNtOpenFile)GetProcAddress(hNtdll, "NtOpenFile");
    g_RtlInitUnicodeString = (pRtlInitUnicodeString)GetProcAddress(hNtdll, "RtlInitUnicodeString");
    g_NtClose = (pNtClose)GetProcAddress(hNtdll, "NtClose");
    return g_NtQueryDirectoryFileEx && g_NtOpenFile && g_RtlInitUnicodeString && g_NtClose;
}