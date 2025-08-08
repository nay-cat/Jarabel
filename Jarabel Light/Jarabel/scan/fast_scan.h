#pragma once

#include "../core/app.h"

#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <immintrin.h> 
#include <winternl.h>

#include "scanner_callbacks.h"

#define NODE_BLOCK_SIZE 256
#define DIR_BUFFER_SIZE (64 * 1024) // approximated from reverse engineering of FIND_FIRST_EX_LARGE_FETCH in the kernel's management of FindFirstFileExW
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif
#ifndef STATUS_NO_MORE_FILES
#define STATUS_NO_MORE_FILES ((NTSTATUS)0x80000006L)
#endif
#ifndef SL_RESTART_SCAN
#define SL_RESTART_SCAN 0x01
#endif

#define OBJ_CASE_INSENSITIVE         0x00000040L
#define FILE_SYNCHRONOUS_IO_NONALERT 0x00000020
#define FILE_DIRECTORY_FILE          0x00000001
#define FILE_OPEN_FOR_BACKUP_INTENT  0x00004000
#define FileBothDirectoryInformation (FILE_INFORMATION_CLASS)3

void RunUnstableScan(FOUND_JAR_CALLBACK callback, void* context);