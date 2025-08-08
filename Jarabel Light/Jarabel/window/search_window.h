#pragma once

#include "../core/app.h"
#include <windows.h>

typedef struct {
    MasterList* classNamesList;
} CachePopulationContext;

LRESULT CALLBACK GlobalSearchWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);