#pragma once

#include "scanner_callbacks.h"
#include "scan.h"

typedef struct {
    int matchCount;
    const WCHAR* searchTerm;
} SearchContext;

unsigned __stdcall JarScanThread(void* __restrict pArguments);
unsigned __stdcall GlobalClassScanThread(void* __restrict pArguments);