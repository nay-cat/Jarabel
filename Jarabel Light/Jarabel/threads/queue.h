#pragma once

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "../core/app.h"

BOOL Queue_Init(WorkQueue* queue);
void Queue_Destroy(WorkQueue* queue);
void Queue_Push(WorkQueue* queue, const WCHAR* path);
BOOL Queue_Pop(WorkQueue* queue, WCHAR* outPath);