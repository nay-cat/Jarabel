#pragma once

#include <windows.h>

LRESULT CALLBACK DarkHeaderSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void SubclassListViewHeader(HWND hListView);
void HandleCustomDraw(LPARAM lParam, LRESULT* pResult);
LRESULT HandleOwnerDraw(LPARAM lParam);
LRESULT HandleMeasureItem(LPARAM lParam);
LRESULT HandleCtlColor(UINT msg, WPARAM wParam);