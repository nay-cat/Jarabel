#pragma once

#include <windows.h>
#include "../core/globals.h"

void HandleJarInspect(HWND hwnd);
void HandleJournalCheck(HWND hwnd);
void HandleRecentDocsCheck(HWND hwnd);
void HandlePrefetchCheck(HWND hwnd);
void HandleProcessScan(HWND hwnd);

void AnalyzeAndCategorizeJar(FileInfo* __restrict pOriginalInfo, HWND hOwnerWnd, ThreadLocalJarLists* __restrict tlsLists);
void ProcessJarFile(HWND hListView, const WCHAR* __restrict jarPath);