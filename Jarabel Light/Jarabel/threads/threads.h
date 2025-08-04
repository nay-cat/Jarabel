#pragma once

#include "ntapi_wrappers.h"
#include "queue.h"

#define BATCH_UPDATE_THRESHOLD 10

typedef enum {
    SCAN_MODE_RECURSIVE, 
    SCAN_MODE_QUEUE     
} ScanMode;

unsigned __stdcall JarScanThread_Multi(void* pArguments);
unsigned __stdcall JarScanThread_Single(void* pArguments);
unsigned __stdcall GlobalClassScanThread(void* pArguments);