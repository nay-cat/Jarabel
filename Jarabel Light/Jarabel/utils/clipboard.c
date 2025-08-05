#include "../core/app.h"

void __cdecl CopyToClipboard(HWND hwnd, const WCHAR* __restrict text, size_t text_len_in_chars) {
    if (!text || text_len_in_chars == 0 || !OpenClipboard(hwnd)) {
        return;
    }

    const size_t len_in_bytes = (text_len_in_chars + 1) * sizeof(WCHAR);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len_in_bytes);

    if (hMem != NULL) {
        void* pMem = GlobalLock(hMem);
        if (pMem != NULL) {
            memcpy(pMem, text, text_len_in_chars * sizeof(WCHAR));

            ((WCHAR*)pMem)[text_len_in_chars] = L'\0';

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