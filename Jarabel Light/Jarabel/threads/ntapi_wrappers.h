#ifndef NTAPI_WRAPPERS_H
#define NTAPI_WRAPPERS_H

#include <windows.h>
#include <winternl.h>

#pragma region NTAPI_DEFINITIONS

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define STATUS_NO_MORE_FILES         ((NTSTATUS)0x80000006L)

#define OBJ_CASE_INSENSITIVE         0x00000040L
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_DIRECTORY_FILE          0x00000001
#define FILE_OPEN_FOR_BACKUP_INTENT  0x00004000

#define FileBothDirectoryInformation (FILE_INFORMATION_CLASS)3

typedef struct _FILE_BOTH_DIR_INFORMATION {
    ULONG         NextEntryOffset;
    ULONG         FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG         FileAttributes;
    ULONG         FileNameLength;
    ULONG         EaSize;
    CCHAR         ShortNameLength;
    WCHAR         ShortName[12];
    WCHAR         FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef NTSTATUS(NTAPI* pNtQueryDirectoryFileEx)(HANDLE, HANDLE, PIO_APC_ROUTINE, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS, ULONG, PUNICODE_STRING);
typedef NTSTATUS(NTAPI* pNtOpenFile)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, ULONG, ULONG);
typedef void(NTAPI* pRtlInitUnicodeString)(PUNICODE_STRING, PCWSTR);
typedef NTSTATUS(NTAPI* pNtClose)(HANDLE);

extern pNtQueryDirectoryFileEx g_NtQueryDirectoryFileEx;
extern pNtOpenFile g_NtOpenFile;
extern pRtlInitUnicodeString g_RtlInitUnicodeString;
extern pNtClose g_NtClose;

#pragma endregion

BOOL InitializeNtApi();

#endif 