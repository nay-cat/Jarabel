#include "../core/app.h"

void __cdecl CopyToClipboard(HWND hwnd, const WCHAR* __restrict text) {
    if (!text || !OpenClipboard(hwnd)) {
        return;
    }

    const size_t len = (wcslen(text) + 1) * sizeof(WCHAR);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);

    if (hMem != NULL) {
        void* pMem = GlobalLock(hMem);
        if (pMem != NULL) {
            memcpy(pMem, text, len);
            GlobalUnlock(hMem);
            EmptyClipboard(); 
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        else {
            GlobalFree(hMem);
        }
    }

    CloseClipboard();
}