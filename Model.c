#define UNICODE
#define _UNICODE

#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <gsl/gsl_blas.h>

#include "Model.h"
#include "ModelResources.h"
#include "ModelSettings.h"
#include "MatrixTransformations.h"
#include "Parser.h"
#include "ThreadPool.h"

#include "DoubleBuffering.h"

#define PTH_OBJ_FILE  "C:\\Users\\Aleksej\\Downloads\\model\\model.obj" 
#define TIMER_ID_15MSEC     1
#define COLOR_IMAGE     RGB(255, 255, 255)

    /* Graphic window */
HWND hwndMainWindow;
HDC hdcBack;
HBITMAP hbmBack;
RECT rcClient;

HTHDPOOL hPool;
HANDLE hTaskExecutedEvent;
typedef struct {
    gsl_vector_int *pVector;
    int j;
} LineStruct;
LineStruct params[10000];

    /* .OBJ file */
FILE *objFile;
ObjFile *pObjFile;
gsl_vector **gvWorldVertices;
gsl_vector **gvPaintVertices;
gsl_block *gbWorldVertices;
gsl_block *gbPaintVertices;

    /* Mouse tracking */
POINT ptMousePrev;
BOOL isActivated;

double x = 0, y = 0, z = -2, angleY = 0, angleX = 0; 

#define widthScreen 1920
#define widthCV 2
#define heightCV 2
#define heightScreen 1080
#define zNear 1
#define zFar 10

double translViewPort[] = 
    {
        heightScreen / 2.0, 0, 0, widthScreen / 2.0, 
        0, - heightScreen / 2.0, 0, heightScreen / 2.0,
        0, 0, 1, 0,
        0, 0, -1, 0
    };
double translProjection[] = 
    {
        2.0 * zNear / widthCV, 0, 0, 0, 
        0, 2.0 * zNear / heightCV, 0, 0,
        0, 0, zFar / (zNear - zFar), zNear * zFar / (zNear - zFar),
        0, 0, -1, 0
    };

gsl_matrix matrixViewPort;
gsl_matrix matrixProjection;
gsl_vector *pResult;

void InitializeResources()
{
    InitializeMatrixTrans();
    objFile = fopen(PTH_OBJ_FILE, "r");
    pObjFile = parseOBJ(objFile);    

    matrixViewPort = gsl_matrix_view_array(translViewPort, 4, 4).matrix;
    matrixProjection = gsl_matrix_view_array(translProjection, 4, 4).matrix;
    pResult = gsl_vector_calloc(4);
    
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
    
    memcpy(gbPaintVertices->data, gbWorldVertices->data, sizeof(double) * gbPaintVertices->size);
    for (int i = 1; i < pObjFile->v->nCurSize; i++)
    {
        gsl_vector *pVector = gvPaintVertices[i];
        
        ApplyMatrix(pVector, MT_MOTION, 0, 0, -2, 0);
        gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixProjection, pVector, 0, pResult);
        gsl_vector_memcpy(pVector, pResult);
        for (int j = 0; j < 3; j++)
        {
            pVector->data[j] /= pVector->data[3];
        }
        pVector->data[3] /= pVector->data[3];
        gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixViewPort, pVector, 0, pResult);
        gsl_vector_memcpy(pVector, pResult);
    }
    // hPool = CreateThreadPool(10000, 1);
    // hTaskExecutedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    // double widthScreen = 1280;
    // double widthCV = 2;
    // double heightCV = 2;
    // double heightScreen = 720;
    // double zNear = 1;
    // double zFar = 10;

    // double translViewPort[] = 
    // {
    //     heightScreen / 2.0, 0, 0, widthScreen / 2.0, 
    //     0, - heightScreen / 2.0, 0, heightScreen / 2.0,
    //     0, 0, 1, 0,
    //     0, 0, -1, 0
    // };
    // double translProjection[] = 
    // {
    //     2.0 * zNear / widthCV, 0, 0, 0, 
    //     0, 2.0 * zNear / heightCV, 0, 0,
    //     0, 0, zFar / (zNear - zFar), zNear * zFar / (zNear - zFar),
    //     0, 0, -1, 0
    // };
    // gsl_matrix matrixViewPort = gsl_matrix_view_array(translViewPort, 4, 4).matrix;
    // gsl_matrix matrixProjection = gsl_matrix_view_array(translProjection, 4, 4).matrix;
    // gsl_vector *pResult = gsl_vector_calloc(4);

    // for (int i = 1; i < pObjFile->v->nCurSize; i++)
    // {
    //     gsl_vector *pVector = pObjFile->v->data[i];
    //     // gsl_vector_printf("")
    //     ApplyMatrix(pObjFile->v->data[i], MT_MOTION, 0, 0, -2, 0);
    //     gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixProjection, pObjFile->v->data[i], 0, pResult);
    //     gsl_vector_memcpy(pObjFile->v->data[i], pResult);
    //     for (int j = 0; j < 3; j++)
    //     {
    //         pVector->data[j] /= pVector->data[3];
    //     }
    //     pVector->data[3] /= pVector->data[3];
    //     gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixViewPort, pObjFile->v->data[i], 0, pResult);
    //     gsl_vector_memcpy(pObjFile->v->data[i], pResult);

    //     // ApplyMatrix(pObjFile->v->data[i], MT_MOTION, 1270/2, 360 / 2 + 200, 0, 0);
    //     // ApplyMatrix(pObjFile->v->data[i], MT_MOTION, 0, 300, 0, 0);
    //     // ApplyMatrix(pObjFile->v->data[i], MT_SCALE, 2, 1, 0, 0);
    //     // ApplyMatrix(pObjFile->v->data[i], MT_Z_ROTATE, 0, 0, 0, 180);
    // }
}

void FreeAllResources()
{
    FinalizeMatrixTrans();
    DestroyObjFileInfo(pObjFile);
    fclose(objFile);
}

inline void DrawCustomLine(HDC dc, gsl_vector *pVector1, gsl_vector *pVector2)
{
    int x1 = (int) gsl_vector_get(pVector1, 0);
    int y1 = (int) gsl_vector_get(pVector1, 1);
    int x2 = (int) gsl_vector_get(pVector2, 0);
    int y2 = (int) gsl_vector_get(pVector2, 1);

    if (x1 < 0 || x2 < 0 || x1 > widthScreen || x2 > widthScreen || y1 < 0 || y2 < 0 || y1 > heightScreen || y2 > heightScreen) return;

    int deltaX = abs(x2 - x1);
    int deltaY = abs(y2 - y1);
    int signX = x2 >= x1 ? 1 : -1;
    int signY = y2 >= y1 ? 1 : -1;

    if (deltaY <= deltaX)
    {
        int d = (deltaY << 1) - deltaX;
        int d1 = deltaY << 1;
        int d2 = (deltaY - deltaX) << 1;

        SetPixel(dc, x1, y1, COLOR_IMAGE);
        // Ellipse(dc, x1, y1, x1+2, y1+2);
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

            SetPixel(dc, x, y, COLOR_IMAGE);
            // Ellipse(dc, x, y, x+2, y+2);
        }
    }
    else
    {
        int d = (deltaX << 1) - deltaY;
        int d1 = deltaX << 1;
        int d2 = (deltaX - deltaY) << 1;

        SetPixel(dc, x1, y1, COLOR_IMAGE);
        // Ellipse(dc, x1, y1, x1+2, y1+2);
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

            SetPixel(dc, x, y, COLOR_IMAGE);
            // Ellipse(dc, x, y, x+2, y+2);
        }

    }

}
 
// void task(LPVOID params)
// {
//     LineStruct *pS = (LineStruct *) params;

//     DrawLine(hdcBack, gvPaintVertices[gsl_vector_int_get(pS->pVector, pS->j)], gvPaintVertices[gsl_vector_int_get(pS->pVector, pS->j + 1)]); //change vector type to int
//     SetEvent(hTaskExecutedEvent);
// }


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        /* Section for window messages */
        case WM_CREATE:
            InitializeResources();
            SetTimer(hWnd, TIMER_ID_15MSEC, 16, NULL);
            break;
        case WM_TIMER:
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        case WM_PAINT:
            HDC dc;
            PAINTSTRUCT ps;

            dc = BeginPaint(hWnd, &ps);

            SaveDC(hdcBack);
            
            FillRect(hdcBack, &rcClient, GetStockBrush(BLACK_BRUSH));
            for (int i = 1; i < pObjFile->fv->nCurSize; i++)
            {
                gsl_vector_int *pVector = pObjFile->fv->data[i];

                for (int j = 0; j < pVector->size - 1; j++)
                {
                    // params[k].j = j;
                    // params[k].pVector = pVector;
                    // SubmitTask(hPool, task, &params[k++]);
                    DrawCustomLine(hdcBack, gvPaintVertices[pVector->data[j]], gvPaintVertices[pVector->data[j + 1]]); //change vector type to int
                }                
                DrawCustomLine(hdcBack, gvPaintVertices[pVector->data[0]], gvPaintVertices[pVector->data[pVector->size - 1]]); //change vector type to int
            }

            // FillRect(hdcBack, &rcClient, GetStockBrush(WHITE_BRUSH));
            // for (int i = 1; i < pObjFile->v->nCurSize; i++)
            // {
            //     gsl_vector *pVector = gvPaintVertices[i];
                
            //     int x = pVector->data[0];
            //     int y = pVector->data[1];

            //     Ellipse(hdcBack, x, y, x+2, y+2);
            // }

            // while (GetExecutedTasksCount(hPool) > 10000)
            // {
            //     WaitForSingleObject(hTaskExecutedEvent, INFINITE);
            // }

            // SetExecutedTasksCount(hPool, 0);
            // ResetEvent(hTaskExecutedEvent);
            RestoreDC(hdcBack, -1);

            BitBlt(dc, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcBack, 0, 0, SRCCOPY);
            EndPaint(hWnd, &ps);
            break;
        case WM_SIZE:
            FinalizeBuffer(&hdcBack, &hbmBack);
            InitializeBuffer(hWnd, &hdcBack, &hbmBack, &rcClient);
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
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);

                angleY += (x - ptMousePrev.x) / 1280.0 * 2 * M_PI;
                angleX += (y - ptMousePrev.y) / 720.0 * 2 * M_PI;

                ptMousePrev.x = x;
                ptMousePrev.y = y;

                memcpy(gbPaintVertices->data, gbWorldVertices->data, sizeof(double) * gbPaintVertices->size);
                for (int i = 1; i < pObjFile->v->nCurSize; i++)
                {
                    gsl_vector *pVector = gvPaintVertices[i];
                    
                    ApplyMatrix(pVector, MT_X_ROTATE, 0, 0, 0, angleX);
                    ApplyMatrix(pVector, MT_Y_ROTATE, 0, 0, 0, angleY);
                    ApplyMatrix(pVector, MT_MOTION, 0, 0, z, 0);
                    gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixProjection, pVector, 0, pResult);
                    gsl_vector_memcpy(pVector, pResult);
                    for (int j = 0; j < 3; j++)
                    {
                        pVector->data[j] /= pVector->data[3];
                    }
                    pVector->data[3] /= pVector->data[3];
                    gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixViewPort, pVector, 0, pResult);
                    gsl_vector_memcpy(pVector, pResult);
                }
                
                // InvalidateRect(hWnd, NULL, FALSE);
            }
            break;
        case WM_MOUSEWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
            {
                z += 0.1;
            }
            else
            {
                z -= 0.1;
            }
            memcpy(gbPaintVertices->data, gbWorldVertices->data, sizeof(double) * gbPaintVertices->size);
                for (int i = 1; i < pObjFile->v->nCurSize; i++)
                {
                    gsl_vector *pVector = gvPaintVertices[i];
                    
                    ApplyMatrix(pVector, MT_X_ROTATE, 0, 0, 0, angleX);
                    ApplyMatrix(pVector, MT_Y_ROTATE, 0, 0, 0, angleY);
                    ApplyMatrix(pVector, MT_MOTION, 0, 0, z, 0);
                    gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixProjection, pVector, 0, pResult);
                    gsl_vector_memcpy(pVector, pResult);
                    for (int j = 0; j < 3; j++)
                    {
                        pVector->data[j] /= pVector->data[3];
                    }
                    pVector->data[3] /= pVector->data[3];
                    gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixViewPort, pVector, 0, pResult);
                    gsl_vector_memcpy(pVector, pResult);
                }
                
                // InvalidateRect(hWnd, NULL, FALSE);
            break;
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
    hwndMainWindow = CreateWindow(_TEXT("AppClass"), APPLICATION_NAME, WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME), CW_USEDEFAULT, 0, widthScreen, heightScreen, NULL, NULL, hInstance, NULL);
    
    ShowWindow(hwndMainWindow, SW_MAXIMIZE);
    UpdateWindow(hwndMainWindow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}