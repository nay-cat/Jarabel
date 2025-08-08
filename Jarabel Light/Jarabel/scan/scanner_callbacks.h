#pragma once
#include <windows.h>

// a callback function pointer that the scanners will use to report found JAR file paths one by one
typedef void (*FOUND_JAR_CALLBACK)(const WCHAR* path, void* context);