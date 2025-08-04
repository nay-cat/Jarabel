#pragma once

#include <windows.h>

void InitImageResource(HINSTANCE hInstance);

void InitGdiResources();

void RegisterWindowClasses(HINSTANCE hInstance);

void FreeGdiResources();

void SetupCustomHighlightColor();

void RestoreOriginalHighlightColor();