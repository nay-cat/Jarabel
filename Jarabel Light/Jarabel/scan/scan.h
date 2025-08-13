#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>
#include <strsafe.h>
#include <pathcch.h>

#pragma comment(lib, "pathcch.lib")

#include "scanner_callbacks.h"

#ifndef _countof
#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#endif

#define THREADS_PER_CPU 2   // worker threads = CPU_COUNT * THREADS_PER_CPU (tune 1..4)
#define MAX_PATH_BUF 32768  // allow long paths via \\?\ prefix (characters)

void ScanSystemJars(FOUND_JAR_CALLBACK callback, void* context);