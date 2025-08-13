#include "../core/app.h"

LRESULT CALLBACK DarkHeaderSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    UNREFERENCED_PARAMETER(dwRefData); UNREFERENCED_PARAMETER(uIdSubclass);

    if (g_isDarkMode) {
        if (uMsg == WM_ERASEBKGND) {
            return 1;
        }
        if (uMsg == WM_PAINT) {
            PAINTSTRUCT ps;
            const HDC hdc = BeginPaint(hWnd, &ps);

            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            FillRect(hdc, &rcClient, g_darkTabBrush);

            const int nItemCount = Header_GetItemCount(hWnd);
            const HPEN hPen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
            const HPEN hOldPen = SelectObject(hdc, hPen);

            SetTextColor(hdc, g_whiteColor);
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_hFont);

            for (int i = 0; i < nItemCount; i++) {
                RECT rcItem = { 0 };
                Header_GetItemRect(hWnd, i, &rcItem);

                WCHAR szText[256] = { 0 };
                const HDITEMW hdi = { .mask = HDI_TEXT | HDI_FORMAT, .pszText = szText, .cchTextMax = 256 };
                Header_GetItem(hWnd, i, &hdi);

                rcItem.left += 8;
                rcItem.right -= 8;
                DrawTextW(hdc, szText, -1, &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                MoveToEx(hdc, rcItem.right + 7, rcItem.top + 4, NULL);
                LineTo(hdc, rcItem.right + 7, rcItem.bottom - 4);
            }

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK TabSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    UNREFERENCED_PARAMETER(uIdSubclass);
    UNREFERENCED_PARAMETER(dwRefData);

    if (g_isDarkMode) {
        if (uMsg == WM_ERASEBKGND) {
            // Prevent default erase to eliminate flicker
            return 1;
        }
        if (uMsg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            // the entire background correctly.
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            // entire control with the color for the tab bar area
            FillRect(hdc, &rcClient, g_darkTabBrush);

            // inner content area (the "page")
            RECT rcPage = rcClient;
            TabCtrl_AdjustRect(hWnd, FALSE, &rcPage);

            // content area with the main dark background color
            if (rcPage.top < rcPage.bottom) {
                FillRect(hdc, &rcPage, g_darkBrush);
            }

            // paint all tab items (the foreground)
            const int iTabCount = TabCtrl_GetItemCount(hWnd);
            const int iSelectedTab = TabCtrl_GetCurSel(hWnd);
            SetBkMode(hdc, TRANSPARENT);

            for (int i = 0; i < iTabCount; i++) {
                RECT rcTab = { 0 };
                TabCtrl_GetItemRect(hWnd, i, &rcTab);

                WCHAR szText[256] = { 0 };
                TCITEMW tci = { .mask = TCIF_TEXT, .pszText = szText, .cchTextMax = _countof(szText) };
                TabCtrl_GetItem(hWnd, i, &tci);

                // Set text color to white for all tabs to ensure visibility
                SetTextColor(hdc, g_whiteColor);
                SelectObject(hdc, (i == iSelectedTab) ? g_hFontBold : g_hFont);

                // Draw the tab label
                RECT rcTabText = rcTab;
                rcTabText.left += 8;
                rcTabText.right -= 8;
                DrawTextW(hdc, szText, -1, &rcTabText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                // the underline highlight for the selected tab
                if (i == iSelectedTab) {
                    RECT rcUnderline = { rcTab.left, rcTab.bottom - 2, rcTab.right, rcTab.bottom };
                    FillRect(hdc, &rcUnderline, g_tabHighlightBrush);
                }
            }

            EndPaint(hWnd, &ps);
            return 0;
        }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void SubclassListViewHeader(const HWND hListView) {
    const HWND hHeader = ListView_GetHeader(hListView);
    if (hHeader) {
        SetWindowSubclass(hHeader, DarkHeaderSubclassProc, 1, 0);
    }
}

void HandleCustomDraw(LPARAM lParam, LRESULT* pResult) {
    //  It remains for any other controls that might use NM_CUSTOMDRAW in the future
    const LPNMCUSTOMDRAW pnmcd = (LPNMCUSTOMDRAW)lParam;
    if (pnmcd->hdr.hwndFrom == hTab && g_isDarkMode) {
        // Since TabSubclassProc now returns 0 on WM_PAINT, this code path will not be executed and yes this is the intended behavior
    }

    *pResult = CDRF_DODEFAULT;
}

LRESULT HandleOwnerDraw(LPARAM lParam) {
    DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;

    if (pdis->CtlID == ID_NIJIKA_IMAGE) {
        if (g_pNijikaPicture) {
            HDC hdc = pdis->hDC;
            long hmWidth, hmHeight;
            g_pNijikaPicture->lpVtbl->get_Width(g_pNijikaPicture, &hmWidth);
            g_pNijikaPicture->lpVtbl->get_Height(g_pNijikaPicture, &hmHeight);
            g_pNijikaPicture->lpVtbl->Render(g_pNijikaPicture, hdc, 0, 0, pdis->rcItem.right, pdis->rcItem.bottom, 0, hmHeight, hmWidth, -hmHeight, &pdis->rcItem);
        }
        return TRUE;
    }

    if (pdis->CtlType == ODT_LISTVIEW && pdis->itemID != (unsigned int)-1) {
        COLORREF textCol = g_isDarkMode ? g_darkTextColor : GetSysColor(COLOR_WINDOWTEXT);

        if (pdis->itemState & ODS_SELECTED) {
            FillRect(pdis->hDC, &pdis->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else {
            FillRect(pdis->hDC, &pdis->rcItem, g_isDarkMode ? g_darkBrush : g_lightBrush);
            SetTextColor(pdis->hDC, textCol);
        }

        SetBkMode(pdis->hDC, TRANSPARENT);
        SelectObject(pdis->hDC, g_hFont);

        if (pdis->CtlID == ID_LISTVIEW_JOURNAL) {
            WCHAR szText[MAX_PATH] = { 0 };
            for (int iCol = 0; iCol < 4; ++iCol) {
                LVITEMW lvi = { 0 };
                lvi.mask = LVIF_TEXT;
                lvi.iItem = pdis->itemID;
                lvi.iSubItem = iCol;
                lvi.pszText = szText;
                lvi.cchTextMax = MAX_PATH;
                ListView_GetItem(pdis->hwndItem, &lvi);

                RECT rcSubItem = { 0 };
                ListView_GetSubItemRect(pdis->hwndItem, pdis->itemID, iCol, LVIR_BOUNDS, &rcSubItem);
                rcSubItem.left += 4;

                if (iCol == 3 && !(pdis->itemState & ODS_SELECTED)) {
                    SetTextColor(pdis->hDC, g_highlightColor);
                }
                DrawTextW(pdis->hDC, szText, -1, &rcSubItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
                if (iCol == 3 && !(pdis->itemState & ODS_SELECTED)) {
                    SetTextColor(pdis->hDC, textCol);
                }
            }
        }
        else {
            FileInfo* pInfo = (FileInfo*)pdis->itemData;
            if (!pInfo) return TRUE;

            RECT rc = pdis->rcItem;
            rc.left += 5; rc.top += 2;

            WCHAR* filename = PathFindFileNameW(pInfo->szFilePath);
            SelectObject(pdis->hDC, g_hFontBold);

            if (pInfo->isObfuscated && !(pdis->itemState & ODS_SELECTED)) {
                SetTextColor(pdis->hDC, RGB(255, 80, 80));
            }
            else {
                SetTextColor(pdis->hDC, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : textCol);
            }

            size_t filename_len = wcsnlen(filename, MAX_PATH - (filename - pInfo->szFilePath));
            TextOutW(pdis->hDC, rc.left, rc.top, filename, (int)filename_len);
            SIZE sz;
            GetTextExtentPoint32W(pdis->hDC, filename, (int)filename_len, &sz);
            rc.left += sz.cx;

            SetTextColor(pdis->hDC, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : textCol);
            SelectObject(pdis->hDC, g_hFont);

            WCHAR szSize[64], szTime[64];
            FormatFileSize(pInfo->liFileSize, szSize, _countof(szSize));
            FormatFileTime(pInfo->ftLastAccessTime, szTime, _countof(szTime));

            SetTextColor(pdis->hDC, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : g_neutralColor);
            WCHAR szDetails[200];
            swprintf_s(szDetails, _countof(szDetails), L" - Size: %s - Last Used: ", szSize);

            size_t szDetails_len = wcsnlen(szDetails, _countof(szDetails));
            TextOutW(pdis->hDC, rc.left, rc.top + 1, szDetails, (int)szDetails_len);
            GetTextExtentPoint32W(pdis->hDC, szDetails, (int)szDetails_len, &sz);
            rc.left += sz.cx;

            SetTextColor(pdis->hDC, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : g_highlightColor);
            size_t szTime_len = wcsnlen(szTime, _countof(szTime));
            TextOutW(pdis->hDC, rc.left, rc.top + 1, szTime, (int)szTime_len);

            rc = pdis->rcItem;
            rc.top += 20; rc.left += 5;
            SetTextColor(pdis->hDC, (pdis->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : g_neutralColor);
            PathCompactPathW(pdis->hDC, pInfo->szFilePath, rc.right - rc.left - 10);

            size_t szFilePath_len = wcsnlen(pInfo->szFilePath, MAX_PATH);
            TextOutW(pdis->hDC, rc.left, rc.top, pInfo->szFilePath, (int)szFilePath_len);
        }
        return TRUE;
    }
    return FALSE;
}

LRESULT HandleMeasureItem(LPARAM lParam) {
    LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
    if (lpmis->CtlType == ODT_LISTVIEW) {
        if (lpmis->CtlID == ID_LISTVIEW_JOURNAL) {
            lpmis->itemHeight = 20;
        }
        else {
            lpmis->itemHeight = 40;
        }
    }
    return TRUE;
}

LRESULT HandleCtlColor(UINT msg, WPARAM wParam) {
    HDC hdc = (HDC)wParam;
    SetTextColor(hdc, g_darkTextColor);

    if (msg == WM_CTLCOLOREDIT) {
        SetBkColor(hdc, RGB(48, 48, 50));
        return (LRESULT)g_darkBtnBrush;
    }

    if (msg == WM_CTLCOLORBTN) {
        SetBkColor(hdc, g_darkBtnColor);
        return (LRESULT)g_darkBtnBrush;
    }

    if (msg == WM_CTLCOLORSTATIC) {
        SetBkColor(hdc, g_darkBgColor);
        return (LRESULT)g_darkBrush;
    }

    SetBkColor(hdc, g_darkBgColor);
    return (LRESULT)g_darkBrush;
}