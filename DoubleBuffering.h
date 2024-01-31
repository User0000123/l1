#ifndef __DOUBLE_BUFFERING_H__
#define __DOUBLE_BUFFERING_H__

#include <windows.h>

void InitializeBuffer(HWND hwnd, HDC* hdcBack, HBITMAP* hbmBack, RECT *rcClient);
void FinalizeBuffer(HDC* hdcBack, HBITMAP* hbmBack);

#endif