#include "DoubleBuffering.h"

void InitializeBuffer(HWND hwnd, HDC* hdcBack, HBITMAP* hbmBack, RECT *rcClient)
{
    HDC hdcWindow;

    GetClientRect(hwnd, rcClient);
    
    hdcWindow = GetDC(hwnd);

    *hdcBack = CreateCompatibleDC(hdcWindow);
    *hbmBack = CreateCompatibleBitmap(hdcWindow, rcClient->right - rcClient->left, rcClient->bottom - rcClient->top);
    SaveDC(*hdcBack);
    SelectObject(*hdcBack, *hbmBack);
    
    ReleaseDC(hwnd, hdcWindow);
}

void FinalizeBuffer(HDC* hdcBack, HBITMAP* hbmBack)
{
    if (*hdcBack)
    {
        RestoreDC(*hdcBack, -1);
        DeleteObject(*hbmBack);
        DeleteDC(*hdcBack);
        *hbmBack = 0;
    }
}