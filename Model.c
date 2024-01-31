#define UNICODE
#define _UNICODE

#include <tchar.h>

#include "Model.h"
#include "ModelResources.h"
#include "ModelSettings.h"

#include "DoubleBuffering.h"

    /* Game window */
HWND hwndMainWindow;
HDC hdcBack;
HBITMAP hbmBack;
RECT rcClient;

void InitializeGameResources()
{
   
}

void FreeAllResources()
{
   
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        /* Section for window messages */
        case WM_CREATE:
            InitializeGameResources();
            break;
        case WM_SIZE:
            FinalizeBuffer(&hdcBack, &hbmBack);
            InitializeBuffer(hWnd, &hdcBack, &hbmBack, &rcClient);
            break;
        case WM_LBUTTONDOWN:
        case WM_KEYDOWN:
            break;
        case WM_DESTROY:
            FreeAllResources();
            FinalizeBuffer(&hdcBack, &hbmBack);
            PostQuitMessage(0);
            break;
    }
    
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI ModelStart(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSEX wcex;
    MSG msg;

    memset(&wcex, 0, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = _TEXT("AppClass");
    wcex.hIconSm = wcex.hIcon;

    RegisterClassExW(&wcex);
    hwndMainWindow = CreateWindow(_TEXT("AppClass"), APPLICATION_NAME, WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME), CW_USEDEFAULT, 0, 1280, 720, NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwndMainWindow, nShowCmd);
    UpdateWindow(hwndMainWindow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}