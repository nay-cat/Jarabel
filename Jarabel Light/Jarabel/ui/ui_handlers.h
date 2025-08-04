#pragma once

#include <windows.h>

void RepopulateActiveList();
void HandleAppResize(LPARAM lParam);
void ToggleDarkMode(HWND hwnd);
void HandleTabChange();
void HandleSmoothScroll(WPARAM wParam);