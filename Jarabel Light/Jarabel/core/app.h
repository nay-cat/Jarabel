#pragma once

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdint.h>
#include <winioctl.h>
#include <shlwapi.h>
#include <process.h>
#include <psapi.h>
#include <olectl.h>
#include <dwmapi.h>
#include <math.h>
#include <string.h>
#include <winternl.h>
#include <intrin.h>
#include <strsafe.h>
#include <wchar.h> 
#include <stdlib.h> 
#include <pathcch.h> 

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "Pathcch.lib") 

#include "globals.h"

#include "../res/ids.h"

#include "../ui/ui_creation.h"
#include "../ui/ui_handlers.h"
#include "../ui/ui_drawing.h"
#include "../actions/action_handlers.h"
#include "../threads/threads.h" 
#include "../window/search_window.h"
#include "../utils/utils.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);