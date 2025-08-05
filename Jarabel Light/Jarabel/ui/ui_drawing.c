#include "../core/app.h"

LRESULT CALLBACK DarkHeaderSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (dwRefData), (uIdSubclass);

    if (g_isDarkMode) {
        if (uMsg == WM_ERASEBKGND) {
            return 1;
        }
        if (uMsg == WM_PAINT) {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            FillRect(hdc, &rcClient, g_darkTabBrush);

            int nItemCount = Header_GetItemCount(hWnd);
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
            HPEN hOldPen = SelectObject(hdc, hPen);

            SetTextColor(hdc, g_darkTextColor);
            SetBkMode(hdc, TRANSPARENT);
            SelectObject(hdc, g_hFont);

            for (int i = 0; i < nItemCount; i++) {
                RECT rcItem = { 0 };
                Header_GetItemRect(hWnd, i, &rcItem);

                WCHAR szText[256] = { 0 };
                HDITEMW hdi = { .mask = HDI_TEXT | HDI_FORMAT, .pszText = szText, .cchTextMax = 256 };
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

void SubclassListViewHeader(HWND hListView) {
    HWND hHeader = ListView_GetHeader(hListView);
    if (hHeader) {
        SetWindowSubclass(hHeader, DarkHeaderSubclassProc, 1, 0);
    }
}

void HandleCustomDraw(LPARAM lParam, LRESULT* pResult) {
    LPNMCUSTOMDRAW pnmcd = (LPNMCUSTOMDRAW)lParam;
    if (pnmcd->hdr.hwndFrom == hTab && g_isDarkMode) {
        switch (pnmcd->dwDrawStage) {
        case CDDS_PREPAINT:
            FillRect(pnmcd->hdc, &pnmcd->rc, g_darkTabBrush);
            *pResult = CDRF_NOTIFYITEMDRAW;
            return;

        case CDDS_ITEMPREPAINT: {
            HDC hdc = pnmcd->hdc;
            int iTab = (int)pnmcd->dwItemSpec;
            int iSelectedTab = TabCtrl_GetCurSel(hTab);

            RECT rcTab = { 0 };
            TabCtrl_GetItemRect(hTab, iTab, &rcTab);

            WCHAR szText[256] = { 0 };
            TCITEMW tci = { .mask = TCIF_TEXT, .pszText = szText, .cchTextMax = 256 };
            TabCtrl_GetItem(hTab, iTab, &tci);

            SetBkMode(hdc, TRANSPARENT);

            if (iTab == iSelectedTab) {
                SetTextColor(hdc, g_whiteColor);
                SelectObject(hdc, g_hFontBold);

                RECT rcUnderline = { rcTab.left, rcTab.bottom - 3, rcTab.right, rcTab.bottom };
                FillRect(hdc, &rcUnderline, g_tabHighlightBrush);
            }
            else {
                SetTextColor(hdc, g_neutralColor);
                SelectObject(hdc, g_hFont);
            }

            rcTab.left += 8;
            rcTab.right -= 8;
            DrawTextW(hdc, szText, -1, &rcTab, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            *pResult = CDRF_SKIPDEFAULT;
            return;
        }
        }
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

    if (pdis->CtlID == ID_CHECKBOX_MULTITHREAD) {
        HDC hdc = pdis->hDC;

        FillRect(hdc, &pdis->rcItem, g_isDarkMode ? g_darkBrush : g_lightBrush);

        RECT chkBoxRect = { pdis->rcItem.left, pdis->rcItem.top, pdis->rcItem.left + 16, pdis->rcItem.bottom };
        UINT uState = DFCS_BUTTONCHECK;
        if (g_useMultiThreading) {
            uState |= DFCS_CHECKED;
        }
        DrawFrameControl(hdc, &chkBoxRect, DFC_BUTTON, uState);

        RECT textRect = pdis->rcItem;
        textRect.left += 20;

        WCHAR szText[100];
        GetWindowTextW(pdis->hwndItem, szText, 100);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, g_isDarkMode ? g_darkTextColor : GetSysColor(COLOR_WINDOWTEXT));
        SelectObject(hdc, g_hFont);

        DrawTextW(hdc, szText, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    if (pdis->CtlType == ODT_LISTVIEW && pdis->itemID != -1) {
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
        SetBkColor(hdc, RGB(50, 50, 50));
        return (LRESULT)g_darkBtnBrush;
    }

    if (msg == WM_CTLCOLORBTN) {
        SetBkColor(hdc, RGB(60, 60, 60));
        return (LRESULT)g_darkBtnBrush;
    }

    if (msg == WM_CTLCOLORSTATIC) {
        SetBkColor(hdc, g_darkBgColor);
        return (LRESULT)g_darkBrush;
    }

    SetBkColor(hdc, g_darkBgColor);
    return (LRESULT)g_darkBrush;
}