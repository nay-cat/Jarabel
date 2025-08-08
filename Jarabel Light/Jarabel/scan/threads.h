#pragma once

#include "stable_scan.hpp"
#include "fast_scan.h"
#include "scanner_callbacks.h"

typedef struct {
    int matchCount;
    const WCHAR* searchTerm;
} SearchContext;

unsigned __stdcall StableJarScanThread(void* __restrict pArguments);
unsigned __stdcall UnstableJarScanThread(void* __restrict pArguments);
unsigned __stdcall GlobalClassScanThread(void* __restrict pArguments);