#ifndef __FPS_H__
#define __FPS_H__

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define FPS_OUT_MAX_LENGTH  30

typedef struct {
    int frames;
    long lastTime;
    TCHAR *out;
} TUtilFPS;
TUtilFPS fps;

void fpsLOG(void)
{
    if (GetTickCount() - fps.lastTime > 1000) 
    {
        _stprintf(fps.out, FPS_OUT_MAX_LENGTH, _TEXT("FPS: %d"), fps.frames);        
        fps.frames = 0;
        fps.lastTime = GetTickCount();
    }
}

#endif