#pragma once

#include <windows.h>

#include "scanner_callbacks.h"

#ifdef __cplusplus
extern "C" {
#endif

	void RunStableScan(FOUND_JAR_CALLBACK callback, void* context);

#ifdef __cplusplus
}
#endif