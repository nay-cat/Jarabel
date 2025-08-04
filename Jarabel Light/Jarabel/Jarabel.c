#include "app_lifecycle.h"
#include "core/app.h"
#include "app_init.h"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    (void)hPrevInstance; (void)lpCmdLine;

    InitializeCriticalSection(&g_listLock);
    if (FAILED(OleInitialize(NULL))) return 1;

    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES };
    InitCommonControlsEx(&icex);

    SetupCustomHighlightColor();
    InitImageResource(hInstance);
    InitGdiResources();
    InitMasterLists();
    RegisterWindowClasses(hInstance);

    const WCHAR CLASS_NAME[] = L"Jarabel";
    HWND hwnd = CreateWindowW(CLASS_NAME, L"Jarabel - Light Mode", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"Window Creation Failed! Make sure the class name in WinMain matches the one in RegisterWindowClasses.", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 2;
    }

    BOOL useDarkMode = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_pNijikaPicture) g_pNijikaPicture->lpVtbl->Release(g_pNijikaPicture);
    OleUninitialize();

    FreeGdiResources();
    FreeMasterLists();

    DeleteCriticalSection(&g_listLock);
    RestoreOriginalHighlightColor();

    return (int)msg.wParam;
}