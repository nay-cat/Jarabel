#include "app_init.h"
#include "core/app.h"
#include "res/resource.h" 
#include "main_window_proc.h" 
#include "window/search_window.h"

void SetupCustomHighlightColor() {
    g_originalHighlightColor = GetSysColor(COLOR_HIGHLIGHT);
    g_originalHighlightTextColor = GetSysColor(COLOR_HIGHLIGHTTEXT);

    int elements[] = { COLOR_HIGHLIGHT, COLOR_HIGHLIGHTTEXT };
    // dark blue (#00184D) and white
    COLORREF colors[] = { RGB(0, 120, 215), RGB(255, 255, 255) };
    SetSysColors(2, elements, colors);
}

void RestoreOriginalHighlightColor() {
    int elements[] = { COLOR_HIGHLIGHT, COLOR_HIGHLIGHTTEXT };
    COLORREF colors[] = { g_originalHighlightColor, g_originalHighlightTextColor };
    SetSysColors(2, elements, colors);
}

void InitImageResource(HINSTANCE hInstance) {
    HRSRC hResInfo = FindResource(hInstance, MAKEINTRESOURCE(IDR_NIJIKA_JPG), "JPG");
    if (hResInfo) {
        HGLOBAL hRes = LoadResource(NULL, hResInfo);
        if (hRes) {
            LPVOID pResData = LockResource(hRes);
            DWORD dwResSize = SizeofResource(NULL, hResInfo);
            if (pResData && dwResSize > 0) {
                HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dwResSize);
                if (hGlobal) {
                    LPVOID pGlobalData = GlobalLock(hGlobal);
                    if (pGlobalData) {
                        memcpy_s(pGlobalData, dwResSize, pResData, dwResSize);
                        GlobalUnlock(hGlobal);

                        IStream* pStream = NULL;
                        if (SUCCEEDED(CreateStreamOnHGlobal(hGlobal, TRUE, &pStream))) {
                            OleLoadPicture(pStream, 0, FALSE, &IID_IPicture, (LPVOID*)&g_pNijikaPicture);
                            pStream->lpVtbl->Release(pStream);
                        }
                    }
                }
            }
        }
    }
}

void InitGdiResources() {
    g_darkBrush = CreateSolidBrush(g_darkBgColor);

    g_darkBtnBrush = CreateSolidBrush(RGB(60, 60, 60));

    g_darkTabBrush = CreateSolidBrush(RGB(45, 45, 45));

    g_tabHighlightBrush = CreateSolidBrush(RGB(0, 120, 215));

    g_lightBrush = GetSysColorBrush(COLOR_WINDOW);

    LOGFONTW lf = { 0 };
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Calibri");
    lf.lfHeight = -14;
    g_hFont = CreateFontIndirectW(&lf);

    lf.lfWeight = FW_BOLD;
    g_hFontBold = CreateFontIndirectW(&lf);

    LOGFONTW lfTitle = { 0 };
    wcscpy_s(lfTitle.lfFaceName, LF_FACESIZE, L"Calibri Light");
    lfTitle.lfHeight = -52;
    g_hFontTitle = CreateFontIndirectW(&lfTitle);
}

void FreeGdiResources() {
    DeleteObject(g_darkBrush);
    DeleteObject(g_darkBtnBrush);
    DeleteObject(g_darkTabBrush);
    DeleteObject(g_tabHighlightBrush);
    DeleteObject(g_hFont);
    DeleteObject(g_hFontBold);
    DeleteObject(g_hFontTitle);
}

void RegisterWindowClasses(HINSTANCE hInstance) {
    const WCHAR CLASS_NAME[] = L"Jarabel";
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = g_darkBrush;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    const WCHAR SEARCH_CLASS_NAME[] = L"GlobalSearchWindowClass";
    WNDCLASSW wcSearch = { 0 };
    wcSearch.lpfnWndProc = GlobalSearchWndProc;
    wcSearch.hInstance = hInstance;
    wcSearch.lpszClassName = SEARCH_CLASS_NAME;
    wcSearch.hbrBackground = g_darkBrush;
    wcSearch.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassW(&wcSearch);
}