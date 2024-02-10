#define UNICODE
#define _UNICODE

#include <gsl/gsl_blas.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <math.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <windowsx.h>

#include "Model.h"
#include "MatrixTransformations.h"
#include "Parser.h"

#include "DoubleBuffering.h"
#include "FpsUtility.h"

#define APPLICATION_NAME    _TEXT("3D Model")

#define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\model\\model.obj" 
// #define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\capybara(1)\\capybara.obj" 

#define BACKGROUND_BRUSH    BLACK_BRUSH
#define COLOR_IMAGE         RGB(255, 255, 255)

#define TIMER_REPAINT_ID    1
#define TIMER_REPAINT_MS    15
#define WHEEL_DELTA_SCALE   0.1
#define MOVEMENT_SPEED      3

   /* Perspective projection */
DOUBLE CAMERA_VIEW_WIDTH =  2;
DOUBLE CAMERA_VIEW_HEIGHT = 2;
#define Z_NEAR              1
#define Z_FAR               10

    /* Graphic window */
HWND hwndMainWindow;
RECT rcClient;
WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

    /* Back buffer */
HDC hdcBack;
HBITMAP hbmBack;

    /* Screen bitmap */
BITMAPINFO bmi;
byte *pBytes;
BITMAP bmp;

    /* Model info */
RECT rcModelInfo;
TCHAR *tcsFpsInfo;

    /* .OBJ file */
FILE *objFile;
ObjFile *pObjFile;

    /* Model temp vertices */
gsl_vector **gvWorldVertices;
gsl_vector **gvPaintVertices;
gsl_block *gbWorldVertices;
gsl_block *gbPaintVertices;

    /* Mouse tracking */
POINT ptMousePrev;
BOOL isActivated;

    /* Camera coordinates */
gsl_vector *eye;
gsl_vector *target;
gsl_vector *up;
DOUBLE destR =          2;
DOUBLE angleThetha =    0;
DOUBLE anglePhi =       0;
byte keys[255];
gsl_vector *straightViewDirection;
gsl_vector *sideViewDirection;

double translViewPort[] = 
    {
        R, 0, 0, R, 
        0, R, 0, R,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

double translProjection[] = 
    {
        2.0 * Z_NEAR / 2.0, 0, 0, 0, 
        0, 2.0 * Z_NEAR / 2.0, 0, 0,
        0, 0, Z_FAR / (Z_NEAR - Z_FAR), Z_NEAR * Z_FAR / (Z_NEAR - Z_FAR),
        0, 0, -1, 0
    };

double translView[] = 
    {
        R, R, R, R, 
        R, R, R, R,
        R, R, R, R,
        0, 0, 0, 1
    };

gsl_matrix *matrixTransformation;
gsl_matrix matrixViewPort;
gsl_matrix matrixProjection;
gsl_matrix matrixView;
gsl_vector *pResult;
gsl_vector *xAxis;
gsl_vector *yAxis;
gsl_vector *zAxis;

inline void vector_cross_product3(gsl_vector *v1, gsl_vector *v2, gsl_vector *result)
{
    result->data[0] = v1->data[1] * v2->data[2] - v1->data[2] * v2->data[1]; 
    result->data[1] = v1->data[2] * v2->data[0] - v1->data[0] * v2->data[2]; 
    result->data[2] = v1->data[0] * v2->data[1] - v1->data[1] * v2->data[0]; 
}

void applyTransformations()
{
    memcpy(gbPaintVertices->data, gbWorldVertices->data, sizeof(double) * gbPaintVertices->size);

    gsl_vector_set_zero(target);
    gsl_vector_set(target, 2, -1);
    gsl_vector_add(target, eye);

    gsl_vector_memcpy(zAxis, eye);
    gsl_vector_sub(zAxis, target);
    gsl_vector_scale(zAxis, 1.0 / gsl_blas_dnrm2(zAxis));

    vector_cross_product3(up, zAxis, xAxis);
    gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis));

    gsl_vector_memcpy(yAxis, up);

    for (int i = 0; i < 3; i++)
    {
        translView[i] = xAxis->data[i];
        translView[4 + i] = yAxis->data[i];
        translView[8 + i] = zAxis->data[i];
    }

    gsl_blas_ddot(xAxis, eye, translView + 3);
    gsl_blas_ddot(yAxis, eye, translView + 7);
    gsl_blas_ddot(zAxis, eye, translView + 11);
    for (int i = 0; i < 3; i++)
    {
        translView[4*i + 3] *= -1;
    }

    gsl_matrix_memcpy(matrixTransformation, &matrixViewPort);

    MatrixMult(matrixTransformation, &matrixProjection);
    MatrixMult(matrixTransformation, &matrixView);
    ApplyMatrixM(matrixTransformation, MT_X_ROTATE, 0, 0, 0, angleThetha);
    ApplyMatrixM(matrixTransformation, MT_Y_ROTATE, 0, 0, 0, anglePhi);

    for (int i = 1; i < pObjFile->v->nCurSize; i++)
    {
        gsl_vector *pVector = gvPaintVertices[i];

        gsl_blas_dgemv(CblasNoTrans, 1.0, matrixTransformation, pVector, 0, pResult);
        gsl_vector_memcpy(pVector, pResult);
        gsl_vector_scale(pVector, 1.0 / gsl_vector_get(pVector, 3));
    }
}

void InitializeResources()
{
    InitializeMatrixTrans();
    objFile = fopen(PTH_OBJ_FILE, "r");
    pObjFile = parseOBJ(objFile);

    matrixViewPort = gsl_matrix_view_array(translViewPort, 4, 4).matrix;
    matrixProjection = gsl_matrix_view_array(translProjection, 4, 4).matrix;
    matrixView = gsl_matrix_view_array(translView, 4, 4).matrix;
    matrixTransformation = gsl_matrix_alloc(4, 4);

    pResult = gsl_vector_calloc(4);
    xAxis = gsl_vector_calloc(4);
    yAxis = gsl_vector_calloc(4);
    zAxis = gsl_vector_calloc(4);

    eye = gsl_vector_calloc(4);
    gsl_vector_set(eye, 2, destR);

    up = gsl_vector_calloc(4);
    gsl_vector_set_basis(up, 1);
    target = gsl_vector_calloc(4);

    straightViewDirection = gsl_vector_alloc(4);
    gsl_vector_set_basis(straightViewDirection, 2);
    sideViewDirection = gsl_vector_alloc(4);
    gsl_vector_set_basis(sideViewDirection, 0);
    
    gbWorldVertices = gsl_block_alloc(pObjFile->v->nCurSize * 4);
    gbPaintVertices = gsl_block_alloc(pObjFile->v->nCurSize * 4);

    gvWorldVertices = calloc(pObjFile->v->nCurSize, sizeof(gsl_vector *));
    gvPaintVertices = calloc(pObjFile->v->nCurSize, sizeof(gsl_vector *));
    
    for (int i = 1; i < pObjFile->v->nCurSize; i++)
    {
        gvPaintVertices[i] = gsl_vector_alloc_from_block(gbPaintVertices, i*4, 4, 1);
        gvWorldVertices[i] = gsl_vector_alloc_from_block(gbWorldVertices, i*4, 4, 1);
        gsl_vector_memcpy(gvWorldVertices[i], pObjFile->v->data[i]);
    }
    
    applyTransformations();

    tcsFpsInfo = calloc(FPS_OUT_MAX_LENGTH, sizeof(TCHAR));
}

void FreeAllResources()
{
    free(tcsFpsInfo);
    free(gvPaintVertices);
    free(gvWorldVertices);

    gsl_block_free(gbPaintVertices);
    gsl_block_free(gbWorldVertices);
    gsl_matrix_free(matrixTransformation);

    free(pResult);
    free(eye);
    free(target);
    free(up);
    free(xAxis);
    free(yAxis);
    free(zAxis);
    free(straightViewDirection);
    free(sideViewDirection);

    free(pBytes);

    DestroyObjFileInfo(pObjFile);
    fclose(objFile);
    FinalizeMatrixTrans();
}

inline void DrawCustomLine(HDC dc, gsl_vector *pVector1, gsl_vector *pVector2)
{
    int x1 = (int) gsl_vector_get(pVector1, 0);
    int y1 = (int) gsl_vector_get(pVector1, 1);
    int x2 = (int) gsl_vector_get(pVector2, 0);
    int y2 = (int) gsl_vector_get(pVector2, 1);

    if (x1 < 0 || x2 < 0 || x1 >= bmp.bmWidth || x2 >= bmp.bmWidth || y1 < 0 || y2 < 0 || y1 >= bmp.bmHeight || y2 >= bmp.bmHeight) return;

    int deltaX = abs(x2 - x1);
    int deltaY = abs(y2 - y1);
    int signX = x2 >= x1 ? 1 : -1;
    int signY = y2 >= y1 ? 1 : -1;
    int offset;

    if (deltaY <= deltaX)
    {
        int d = (deltaY << 1) - deltaX;
        int d1 = deltaY << 1;
        int d2 = (deltaY - deltaX) << 1;

        offset = (y1 * bmp.bmWidth + x1) * 4;
        pBytes[offset + 0] = 255;
        pBytes[offset + 1] = 255;
        pBytes[offset + 2] = 255;

        for (int x = x1 + signX, y = y1, i = 1; i < deltaX; i++, x += signX)
        {
            if (d > 0)
            {
                d += d2;
                y += signY;
            }
            else
            {
                d += d1;
            }

        offset = (y * bmp.bmWidth + x) * 4;
        pBytes[offset + 0] = 255;
        pBytes[offset + 1] = 255;
        pBytes[offset + 2] = 255;
        }
    }
    else
    {
        int d = (deltaX << 1) - deltaY;
        int d1 = deltaX << 1;
        int d2 = (deltaX - deltaY) << 1;

        offset = (y1 * bmp.bmWidth + x1) * 4;
        pBytes[offset + 0] = 255;
        pBytes[offset + 1] = 255;
        pBytes[offset + 2] = 255;

        for (int x = x1, y = y1 + signY, i = 1; i <= deltaY; i++, y += signY)
        {
            if (d > 0)
            {
                d += d2;
                x += signX;
            }
            else
            {
                d += d1;
            }
            
            offset = (y * bmp.bmWidth + x) * 4;
            pBytes[offset + 0] = 255;
            pBytes[offset + 1] = 255;
            pBytes[offset + 2] = 255;
        }
    }
}

void SetFullScreen(HWND hwnd)
{
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
    
    if (dwStyle & WS_OVERLAPPEDWINDOW) {
        MONITORINFO mi = { sizeof(mi) };

        if (GetWindowPlacement(hwnd, &g_wpPrev) && GetMonitorInfo(MonitorFromWindow(hwnd,MONITOR_DEFAULTTOPRIMARY), &mi)) 
        {
            SetWindowLong(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_wpPrev);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void DrawProc(HDC hdc)
{
    SaveDC(hdc);
            
    FillRect(hdc, &rcClient, GetStockBrush(BACKGROUND_BRUSH));

    GetDIBits(hdc, hbmBack, 0, bmp.bmHeight, pBytes, &bmi, DIB_RGB_COLORS);
    
    for (int i = 1; i < pObjFile->fv->nCurSize; i++)
    {
        gsl_vector_int *pVector = pObjFile->fv->data[i];

        for (int j = 0; j < pVector->size - 1; j++)
        {
            DrawCustomLine(hdc, gvPaintVertices[pVector->data[j]], gvPaintVertices[pVector->data[j + 1]]); 
        }                
        DrawCustomLine(hdc, gvPaintVertices[pVector->data[0]], gvPaintVertices[pVector->data[pVector->size - 1]]);
    }

    SetDIBits(hdc, hbmBack, 0, bmp.bmHeight, pBytes, &bmi, DIB_RGB_COLORS);

    // FPS counter //////////////////////////////////////
    SetTextColor(hdc, COLOR_IMAGE);
    SetBkColor(hdc, TRANSPARENT);
    DrawText(hdc, tcsFpsInfo, -1, &rcModelInfo, DT_CENTER);
    /////////////////////////////////////////////////////

    RestoreDC(hdc, -1);
}

void MoveProc()
{
    static long lastTime = 0;
    double cameraSpeed = (GetTickCount() - lastTime) / 1000.0 * MOVEMENT_SPEED;

    if (keys[0x57])
    {
        gsl_vector_set(straightViewDirection, 2, -cameraSpeed);
        gsl_vector_add(eye,  straightViewDirection);
    }

    if (keys[0x53])
    {
        gsl_vector_set(straightViewDirection, 2, 1 * cameraSpeed);
        gsl_vector_add(eye,  straightViewDirection);
    }

    if (keys[0x41])
    {
        gsl_vector_set(sideViewDirection, 0, -1 * cameraSpeed);
        gsl_vector_add(eye, sideViewDirection);
    }

    if (keys[0x44])
    {
        gsl_vector_set(sideViewDirection, 0, 1 * cameraSpeed);
        gsl_vector_add(eye, sideViewDirection);
    }

    lastTime = GetTickCount();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        /* Section for window messages */
        case WM_CREATE:
            InitializeResources();
            SetTimer(hWnd, TIMER_REPAINT_ID, TIMER_REPAINT_MS, NULL);
            break;
        case WM_TIMER:
            MoveProc();
            applyTransformations();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case WM_PAINT:
            HDC dc;
            PAINTSTRUCT ps;

            dc = BeginPaint(hWnd, &ps);
            
            DrawProc(hdcBack);        
            
            BitBlt(dc, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcBack, 0, 0, SRCCOPY);
            EndPaint(hWnd, &ps);
            
            fps.frames++;
            break;
        case WM_SIZE:
            FinalizeBuffer(&hdcBack, &hbmBack);
            InitializeBuffer(hWnd, &hdcBack, &hbmBack, &rcClient);
            
            rcModelInfo.left = rcClient.left;
            rcModelInfo.top = rcClient.bottom - 30;
            rcModelInfo.bottom = rcClient.bottom;
            rcModelInfo.right = rcClient.left + 80;

            GetObject(hbmBack, sizeof(BITMAP), &bmp);

            memset(&bmi, 0, sizeof(bmi));
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            GetDIBits(hdcBack, hbmBack, 0, bmp.bmHeight, NULL, &bmi, DIB_RGB_COLORS);
            bmi.bmiHeader.biCompression = BI_RGB;
            bmi.bmiHeader.biHeight = -bmp.bmHeight;

            double scaleX = bmp.bmWidth / 2.0;
            double scaleY = bmp.bmHeight / 2.0;

            translViewPort[0] = scaleY;
            translViewPort[3] = scaleX;
            translViewPort[5] = -scaleY;
            translViewPort[7] = scaleY;

            pBytes = realloc(pBytes, bmi.bmiHeader.biSizeImage);
            break;
        case WM_LBUTTONDOWN: 
            ptMousePrev.x = GET_X_LPARAM(lParam);
            ptMousePrev.y = GET_Y_LPARAM(lParam);
            isActivated = TRUE;
            break;     
        case WM_LBUTTONUP:
            isActivated = FALSE;
            break;
        case WM_MOUSEMOVE: 
            if (isActivated)
            {
                int xMousePos = GET_X_LPARAM(lParam);
                int yMousePos = GET_Y_LPARAM(lParam);

                angleThetha += (yMousePos - ptMousePrev.y) / 720.0 * 2 * M_PI;
                anglePhi += (xMousePos - ptMousePrev.x) / 1280.0 * 2 * M_PI;            

                ptMousePrev.x = xMousePos;
                ptMousePrev.y = yMousePos;
            }
            break;
        case WM_MOUSEWHEEL:
            byte isScalePositive = GET_WHEEL_DELTA_WPARAM(wParam) < 0;

            CAMERA_VIEW_HEIGHT += isScalePositive ? WHEEL_DELTA_SCALE : -WHEEL_DELTA_SCALE;
            CAMERA_VIEW_WIDTH += isScalePositive ? WHEEL_DELTA_SCALE : -WHEEL_DELTA_SCALE;

            translProjection[0] = 2.0 * Z_NEAR / CAMERA_VIEW_WIDTH;
            translProjection[5] = 2.0 * Z_NEAR / CAMERA_VIEW_HEIGHT;

            break;
        case WM_KEYDOWN:    
            keys[wParam] = 1;
            break;
        case WM_KEYUP:
            keys[wParam] = 0;
            break;
        case WM_SYSKEYDOWN:
            if (wParam == VK_F12 && (lParam & (1 << 29)))
            {
                SetFullScreen(hWnd);    
            }
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
    hwndMainWindow = CreateWindow(_TEXT("AppClass"), APPLICATION_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 800, 600, NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwndMainWindow, nShowCmd);
    UpdateWindow(hwndMainWindow);

    fps.out = tcsFpsInfo;
    fps.lastTime = GetTickCount();

    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
        fpsLOG();
    }

    return (int) msg.wParam;
}