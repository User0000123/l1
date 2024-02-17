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
#include "ThreadPool.h"

#include "DoubleBuffering.h"
#include "FpsUtility.h"

#define APPLICATION_NAME    _TEXT("3D Model")

// #define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\model2.obj" 
#define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\model\\model.obj" 
// #define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\capybara(1)\\capybara.obj" 

#define SWAP(a, b)          {typeof(a) temp = a; a = b; b = temp;}
#define SWAP_VECTORS(a, b)  {double temp[4]; memcpy(temp, ((gsl_vector *)a)->data, sizeof(double) * 4); gsl_vector_memcpy(a, b); memcpy(((gsl_vector *)b)->data, temp, sizeof(double) * 4);}
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
BITMAP bmp;
byte *pBytes;

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
gsl_vector *xAxis;
gsl_vector *yAxis;
gsl_vector *zAxis;
DOUBLE destR =          2;
DOUBLE angleThetha =    M_PI_2 - 0.3;
DOUBLE anglePhi =       M_PI_2 - 0.3;

    /* Light */
gsl_vector *light;

    /* Z buffer */
double *zBuffer;

    /* Model transformation matrices */
gsl_matrix *matrixTransformation;
gsl_matrix matrixViewPort;
gsl_matrix matrixProjection;
gsl_matrix matrixView;

    /* Just a temp value for single-threaded computing */
gsl_vector *pResult;

    /* Keyboard */
byte keys[255];

    /* Multithreading */
HTHDPOOL hPool;
typedef struct {
    HDC dc;
    int from;
    int to;
    gsl_vector *pV0;
    gsl_vector *pV1;
    gsl_vector *pV2;
    gsl_vector *pA;
    gsl_vector *pB;
    gsl_vector *pP;
    gsl_vector *norm;
} PARAMS;
PARAMS params[6];
HANDLE hTaskExecutedEvent;

    /* Arrays for wrapper matrices */
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


inline void vector_cross_product3(gsl_vector *v1, gsl_vector *v2, gsl_vector *result)
{
    result->data[0] = v1->data[1] * v2->data[2] - v1->data[2] * v2->data[1]; 
    result->data[1] = v1->data[2] * v2->data[0] - v1->data[0] * v2->data[2]; 
    result->data[2] = v1->data[0] * v2->data[1] - v1->data[1] * v2->data[0]; 
}

void ApplyTransformations()
{
    memcpy(gbPaintVertices->data, gbWorldVertices->data, sizeof(double) * gbPaintVertices->size);

    // View matrix creation
    gsl_vector_memcpy(zAxis, eye);
    gsl_vector_sub(zAxis, target);
    gsl_vector_scale(zAxis, 1.0 / gsl_blas_dnrm2(zAxis));

    vector_cross_product3(up, zAxis, xAxis);
    gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis));

    vector_cross_product3(zAxis, xAxis, yAxis);
    gsl_vector_scale(yAxis, 1.0 / gsl_blas_dnrm2(yAxis));

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
    ///////////////////////////////////////////////
    
    // Creation global transformation matrix
    gsl_matrix_memcpy(matrixTransformation, &matrixViewPort);

    MatrixMult(matrixTransformation, &matrixProjection);
    MatrixMult(matrixTransformation, &matrixView);
    // Also, there may be multiplication by the tranformation matix 
    ////////
    
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
    gsl_vector_set(eye, 3, 1);

    up = gsl_vector_calloc(4);
    gsl_vector_set_basis(up, 1);
    target = gsl_vector_calloc(4);
    
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
    
    ApplyTransformations();

    tcsFpsInfo = calloc(FPS_OUT_MAX_LENGTH, sizeof(TCHAR));

    hPool = CreateThreadPool(100, 6);
    hTaskExecutedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    pV0 = gsl_vector_calloc(4);
    pV1 = gsl_vector_calloc(4); 
    pV2 = gsl_vector_calloc(4);
    pA = gsl_vector_calloc(4);
    pB = gsl_vector_calloc(4);
    pP = gsl_vector_calloc(4);
    light = gsl_vector_calloc(4);
    norm = gsl_vector_calloc(4);

    for (int i = 0; i < 6; i++)
    {
        params[i].pV0 = gsl_vector_calloc(4);
        params[i].pV1 = gsl_vector_calloc(4); 
        params[i].pV2 = gsl_vector_calloc(4);
        params[i].pA = gsl_vector_calloc(4);
        params[i].pB = gsl_vector_calloc(4);
        params[i].pP = gsl_vector_calloc(4);
        params[i].norm = gsl_vector_calloc(4);
    }
}

void FreeAllResources()
{
    // DestroyThreadPool(hPool);
    CloseHandle(hTaskExecutedEvent);
    free(tcsFpsInfo);
    free(gvPaintVertices);
    free(gvWorldVertices);

    gsl_block_free(gbPaintVertices);
    gsl_block_free(gbWorldVertices);
    gsl_matrix_free(matrixTransformation);
    gsl_vector_free(pA);
    gsl_vector_free(pB);
    gsl_vector_free(pP);
    gsl_vector_free(pV0);
    gsl_vector_free(pV1);
    gsl_vector_free(pV2);
    gsl_vector_free(light);
    gsl_vector_free(norm);

    // for (int i = 0; i < 6; i++)
    // {
    //     gsl_vector_free(params[i].pA);
    //     gsl_vector_free(params[i].pB);
    //     gsl_vector_free(params[i].pV0);
    //     gsl_vector_free(params[i].pV1);
    //     gsl_vector_free(params[i].pV2);
    // }

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
    free(zBuffer);

    DestroyObjFileInfo(pObjFile);
    fclose(objFile);
    FinalizeMatrixTrans();
    KillTimer(hwndMainWindow, TIMER_REPAINT_ID);
}

inline void DrawLine(HDC dc, int x0, int y0, int x1, int y1)
{
    BOOL isTranspose = FALSE;

    if (x0 < 0 || x1 < 0 || x0 >= bmp.bmWidth || x1 >= bmp.bmWidth || y0 < 0 || y1 < 0 || y0 >= bmp.bmHeight || y1 >= bmp.bmHeight) return;

    if (abs(x1 - x0) < abs (y1 - y0))
    {
        SWAP(x0, y0);
        SWAP(x1, y1);
        isTranspose = TRUE;
    }

    if (x0 > x1)
    {
        SWAP(x0, x1);
        SWAP(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int signY = y1 >= y0 ? 1 : -1;
    int dxDouble = dx << 1;
    int error = 0;
    int offset;
    int bmWidth = bmp.bmWidth;
    int derror = abs(dy) << 1;

    for (int x = x0, y = y0; x <= x1; x++)
    {
        offset = (y * bmWidth + x) << 2;
        if (isTranspose)
        {
            offset = (x * bmWidth + y) << 2;
        }
        pBytes[offset + 0] = 255;
        pBytes[offset + 1] = 255;
        pBytes[offset + 2] = 255;
        // InterlockedOr((long *)(pBytes + offset), 0x00FFFFFF);

        error += derror;

        if (error > dx)
        {
            y += signY;
            error -= dxDouble;
        }
    }
}

// inline void DrawTriangle(HDC dc, gsl_vector_int *pTriangleVertices, PARAMS *params)
// {
//     gsl_vector_memcpy(params->pV0, gvPaintVertices[pTriangleVertices->data[0]]);
//     gsl_vector_memcpy(params->pV1, gvPaintVertices[pTriangleVertices->data[1]]);
//     gsl_vector_memcpy(params->pV2, gvPaintVertices[pTriangleVertices->data[2]]);

//     if (params->pV0->data[1] == params->pV1->data[1] && params->pV0->data[1] == params->pV2->data[1]) return;

//     if (params->pV0->data[1] > params->pV1->data[1]) SWAP_VECTORS(params->pV0, params->pV1);
//     if (params->pV0->data[1] > params->pV2->data[1]) SWAP_VECTORS(params->pV0, params->pV2);
//     if (params->pV1->data[1] > params->pV2->data[1]) SWAP_VECTORS(params->pV1, params->pV2);

//     int total_height = params->pV2->data[1] - params->pV0->data[1];

//     for (int i=0; i<total_height; i++) {
//         BOOL second_half = i>params->pV1->data[1]-params->pV0->data[1] || params->pV1->data[1]==params->pV0->data[1];
//         int segment_height = second_half ? params->pV2->data[1] - params->pV1->data[1] : params->pV1->data[1] - params->pV0->data[1];
//         float alpha = (float) i / total_height;
//         float beta  = (float)(i - (second_half ? params->pV1->data[1] - params->pV0->data[1] : 0)) / segment_height;
        
//         gsl_vector_memcpy(params->pA, params->pV2);
        
//         gsl_vector_sub(params->pA, params->pV0);
//         // __m256d vec_a = _mm256_loadu_pd(pA->data); // Загружаем первый массив в регистр SIMD
//         // __m256d vec_b = _mm256_loadu_pd(pV0->data); // Загружаем второй массив в другой регистр SIMD
//         // __m256d vec_sum = _mm256_sub_pd(vec_a, vec_b); // Выполняем SIMD-сложение
//         // _mm256_storeu_pd(pA->data, vec_sum); // Сохраняем результат обратно в память
        
//         gsl_vector_scale(params->pA, alpha);

//         gsl_vector_add(params->pA, params->pV0);
//         // vec_a = _mm256_loadu_pd(pA->data); // Загружаем первый массив в регистр SIMD
//         // vec_b = _mm256_loadu_pd(pV0->data); // Загружаем второй массив в другой регистр SIMD
//         // vec_sum = _mm256_add_pd(vec_a, vec_b); // Выполняем SIMD-сложение
//         // _mm256_storeu_pd(pA->data, vec_sum); // 

//         if (second_half)
//         {
//             gsl_vector_memcpy(params->pB, params->pV2);
//             gsl_vector_sub(params->pB, params->pV1);
//             // vec_a = _mm256_loadu_pd(pB->data); // Загружаем первый массив в регистр SIMD
//             // vec_b = _mm256_loadu_pd(pV1->data); // Загружаем второй массив в другой регистр SIMD
//             // vec_sum = _mm256_sub_pd(vec_a, vec_b); // Выполняем SIMD-сложение
//             // _mm256_storeu_pd(pB->data, vec_sum); // 

//             gsl_vector_scale(params->pB, beta);
//             gsl_vector_add(params->pB, params->pV1);
//             // vec_a = _mm256_loadu_pd(pB->data); // Загружаем первый массив в регистр SIMD
//             // vec_b = _mm256_loadu_pd(pV1->data); // Загружаем второй массив в другой регистр SIMD
//             // vec_sum = _mm256_add_pd(vec_a, vec_b); // Выполняем SIMD-сложение
//             // _mm256_storeu_pd(pB->data, vec_sum); //
//         }
//         else
//         {
//             gsl_vector_memcpy(params->pB, params->pV1);
//             gsl_vector_sub(params->pB, params->pV0);
//             // vec_a = _mm256_loadu_pd(pB->data); // Загружаем первый массив в регистр SIMD
//             // vec_b = _mm256_loadu_pd(pV0->data); // Загружаем второй массив в другой регистр SIMD
//             // vec_sum = _mm256_sub_pd(vec_a, vec_b); // Выполняем SIMD-сложение
//             // _mm256_storeu_pd(pB->data, vec_sum); // 

//             gsl_vector_scale(params->pB, beta);
//             gsl_vector_add(params->pB, params->pV0);
//             // vec_a = _mm256_loadu_pd(pB->data); // Загружаем первый массив в регистр SIMD
//             // vec_b = _mm256_loadu_pd(pV0->data); // Загружаем второй массив в другой регистр SIMD
//             // vec_sum = _mm256_add_pd(vec_a, vec_b); // Выполняем SIMD-сложение
//             // _mm256_storeu_pd(pB->data, vec_sum); //
//         }
        
//         int offset;
//         if (params->pA->data[0] > params->pB->data[0]) SWAP_VECTORS(params->pA, params->pB);
//         for (int j = params->pA->data[0]; j <= params->pB->data[0]; j++) {
//             if (j < 0 || j >= bmp.bmWidth || ((int) params->pV0->data[1] + i) < 0 || ((int) params->pV0->data[1] + i) >= bmp.bmHeight) continue;
//             // offset = (((int) pV0->data[1] + i) * bmp.bmWidth + j) << 2;
//             InterlockedOr((long *)(pBytes + offset), 0x00FFFFFF);
//             // pBytes[offset + 0] = 255;
//             // pBytes[offset + 1] = 255;
//             // pBytes[offset + 2] = 255;
//         }
//     }
// }

inline void DrawTriangle(HDC dc, gsl_vector_int *pTriangleVertices, byte r, byte g, byte b)
{
    gsl_vector_memcpy(pV0, gvPaintVertices[pTriangleVertices->data[0]]);
    gsl_vector_memcpy(pV1, gvPaintVertices[pTriangleVertices->data[1]]);
    gsl_vector_memcpy(pV2, gvPaintVertices[pTriangleVertices->data[2]]);

    if (pV0->data[1] > pV1->data[1]) SWAP_VECTORS(pV0, pV1);
    if (pV0->data[1] > pV2->data[1]) SWAP_VECTORS(pV0, pV2);
    if (pV1->data[1] > pV2->data[1]) SWAP_VECTORS(pV1, pV2);

    for (int i = 0; i < 3; i++)
    {
        pV0->data[0] = round(pV0->data[0]);
        pV0->data[1] = round(pV0->data[1]);
        pV1->data[0] = round(pV1->data[0]);
        pV1->data[1] = round(pV1->data[1]);
        pV2->data[0] = round(pV2->data[0]);
        pV2->data[1] = round(pV2->data[1]);
    }
    // if (pV0->data[1] == pV1->data[1] && pV0->data[1] == pV2->data[1]) return;

    int total_height = pV2->data[1] - pV0->data[1];
    // r = rand() % 256;
    // g = rand() % 256;
    // b = rand() % 256;
    for (int i=0; i<total_height; i++) {
        BOOL second_half = i>pV1->data[1]-pV0->data[1] || pV1->data[1]==pV0->data[1];
        int segment_height = (second_half ? pV2->data[1] - pV1->data[1] : pV1->data[1] - pV0->data[1]);
        float alpha = (float) i / total_height;
        float beta  = (float)(i - (second_half ? pV1->data[1] - pV0->data[1] : 0)) / segment_height;
        
        gsl_vector_memcpy(pA, pV2);
        gsl_vector_sub(pA, pV0);
        gsl_vector_scale(pA, alpha);
        gsl_vector_add(pA, pV0);    

        if (second_half)
        {
            gsl_vector_memcpy(pB, pV2);
            gsl_vector_sub(pB, pV1);
            gsl_vector_scale(pB, beta);
            gsl_vector_add(pB, pV1);
        }
        else
        {
            gsl_vector_memcpy(pB, pV1);
            gsl_vector_sub(pB, pV0);
            gsl_vector_scale(pB, beta);
            gsl_vector_add(pB, pV0);
        }
        
        int offset;
        if (pA->data[0] > pB->data[0]) SWAP_VECTORS(pA, pB);
        for (int j = pA->data[0]; j <= pB->data[0]; j++) {
            if (j < 0 || j >= bmp.bmWidth || ((int) pV0->data[1] + i) < 0 || ((int) pV0->data[1] + i) >= bmp.bmHeight) continue;
            float phi = abs(1 - pB->data[0]/pA->data[0]) < 1e-3 ? 1.0 : (float)(j-pA->data[0])/(float)(pB->data[0]-pA->data[0]);
            gsl_vector_memcpy(pP, pB);
            gsl_vector_sub(pP, pA);
            gsl_vector_scale(pP, phi);
            gsl_vector_add(pP, pA);

            offset = (((int) pV0->data[1] + i) * bmp.bmWidth + j) << 2;
            int idx = j+(pV0->data[1] + i)*bmp.bmWidth;
            if (zBuffer[idx] > pP->data[2]) {
                zBuffer[idx] = pP->data[2];

                pBytes[offset + 0] = r;
                pBytes[offset + 1] = g;
                pBytes[offset + 2] = b;    
            }
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

// void task(LPVOID params)
// {
//     PARAMS *param = (PARAMS *) params;
//     for (int i = param->from; i < param->to; i++)
//     {
//         gsl_vector_int *pVector = pObjFile->fv->data[i];

//         DrawTriangle(param->dc, pVector, param);
//     }
//     SetEvent(hTaskExecutedEvent);
//     // ExitThread(0);
//     // InterlockedAdd(&executed, 1);
//     // return 0;
// }
void task(LPVOID params)
{
    PARAMS *param = (PARAMS *) params;
    for (int i = param->from; i < param->to; i++)
    {
        gsl_vector_int *pVector = pObjFile->fv->data[i];
        gsl_vector *pV1, *pV2;

        for (int j = 0; j < pVector->size - 1; j++)
        {
            pV1 = gvPaintVertices[pVector->data[j]];
            pV2 = gvPaintVertices[pVector->data[j + 1]];

            DrawLine(param->dc, pV1->data[0], pV1->data[1], pV2->data[0], pV2->data[1]); 
        }       
        pV1 = gvPaintVertices[pVector->data[0]];
        pV2 = gvPaintVertices[pVector->data[pVector->size - 1]];         
        DrawLine(param->dc, pV1->data[0], pV1->data[1], pV2->data[0], pV2->data[1]); 
    }
    SetEvent(hTaskExecutedEvent);
    // ExitThread(0);
    // InterlockedAdd(&executed, 1);
    // return 0;
}
HBRUSH hbr;
void DrawProc(HDC hdc)
{
    SaveDC(hdc);
            
    FillRect(hdc, &rcClient, GetStockBrush(BACKGROUND_BRUSH));

    GetDIBits(hdc, hbmBack, 0, bmp.bmHeight, pBytes, &bmi, DIB_RGB_COLORS);
    
    // memset(zBuffer,-10, sizeof(double) * bmp.bmWidth * bmp.bmHeight);
    for (int i = 0; i < bmp.bmWidth * bmp.bmHeight; i++)
    {
        zBuffer[i] = 10;
    }
    gsl_vector_memcpy(light, target);
    gsl_vector_sub(light, eye);
    gsl_vector_scale(light, 1.0 / gsl_blas_dnrm2(light));
    srand(100);

    for (int i = 1; i < pObjFile->fv->nCurSize; i++)
    {
        gsl_vector_int *pVector = pObjFile->fv->data[i];

        // for (int j = 0; j < pVector->size - 1; j++)
        // {
        //     pV1 = gvWorldVertices[pVector->data[j]];
        //     pV2 = gvWorldVertices[pVector->data[j + 1]];

        //     // gsl_vector_fprintf(stdout, pV1, "%lf");
        //     // gsl_vector_fprintf(stdout, pV2, "%lf");
        //     DrawLine(hdc, pV1->data[0], pV1->data[1], pV2->data[0], pV2->data[1]); 
        // }       
        // pV1 = gvPaintVertices[pVector->data[0]];
        // pV2 = gvPaintVertices[pVector->data[pVector->size - 1]];         
        // DrawLine(hdc, pV1->data[0], pV1->data[1], pV2->data[0], pV2->data[1]);
        gsl_vector_memcpy(pV0, gvWorldVertices[pVector->data[0]]);
        gsl_vector_memcpy(pV1, gvWorldVertices[pVector->data[1]]);
        gsl_vector_memcpy(pV2, gvWorldVertices[pVector->data[2]]);

        // if (pV0->data[1] > pV1->data[1]) SWAP_VECTORS(pV0, pV1);
        // if (pV0->data[1] > pV2->data[1]) SWAP_VECTORS(pV0, pV2);
        // if (pV1->data[1] > pV2->data[1]) SWAP_VECTORS(pV1, pV2);

        gsl_vector_memcpy(pA, pV1);
        gsl_vector_sub(pA, pV0);

        gsl_vector_memcpy(pB, pV2);
        gsl_vector_sub(pB, pV0);

        // if (pV2->data[0] < pV1->data[0]) SWAP_VECTORS(pA, pB);
        vector_cross_product3(pB, pA, norm);
        gsl_vector_scale(norm, 1.0 / gsl_blas_dnrm2(norm));
        double intens;
        gsl_blas_ddot(light, norm, &intens); 
        if (intens > 0)
        {
            DrawTriangle(hdc, pVector, 255 * intens, 255 * intens, 255 * intens);
        }
    }

    // int delta = pObjFile->fv->nCurSize / 6;
    // params[0].from = 1;
    // params[0].to = 1 + delta;
    // params[0].dc = hdc;
    // SubmitTask(hPool, task, params);
    // // CreateThread(NULL, 0, task, params, 0, NULL);
    // for (int i = 1; i < 5; i++)
    // {
    //     params[i].from = params[i - 1].to;
    //     params[i].to = params[i].from + delta;
    //     params[i].dc = hdc;
    //     SubmitTask(hPool, task, params + i);
    //     // CreateThread(NULL, 0, task, params + i, 0, NULL);
    // }
    // params[5].from = params[4].to;
    // params[5].to = pObjFile->fv->nCurSize;
    // params[5].dc = hdc;
    // SubmitTask(hPool, task, params + 5);
    // CreateThread(NULL, 0, task, params + 5, 0, NULL);

    // int delta = pObjFile->fv->nCurSize / 6;
    // params[0].from = 1;
    // params[0].to = 1 + delta;
    // params[0].dc = hdc;
    // SubmitTask(hPool, task, params);
    // // CreateThread(NULL, 0, task, params, 0, NULL);
    // for (int i = 1; i < 5; i++)
    // {
    //     params[i].from = params[i - 1].to;
    //     params[i].to = params[i].from + delta;
    //     params[i].dc = hdc;
    //     SubmitTask(hPool, task, params + i);
    //     // CreateThread(NULL, 0, task, params + i, 0, NULL);
    // }
    // params[5].from = params[4].to;
    // params[5].to = pObjFile->fv->nCurSize;
    // params[5].dc = hdc;
    // SubmitTask(hPool, task, params + 5);
    // // CreateThread(NULL, 0, task, params + 5, 0, NULL);
    
    // while (executed < 6)
    // {

    // }
    // executed = 0;
    // while (GetExecutedTasksCount(hPool) < 6)
    // {
    //     WaitForSingleObject(hTaskExecutedEvent, INFINITE);
    // }
    // SetExecutedTasksCount(hPool, 0);
    
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
        gsl_vector_memcpy(pResult, target);
        gsl_vector_sub(pResult, eye);
        gsl_vector_scale(pResult, 1 / gsl_blas_dnrm2(pResult) * cameraSpeed);
        gsl_vector_add(eye, pResult);
    }

    if (keys[0x53])
    {
        gsl_vector_memcpy(pResult, target);
        gsl_vector_sub(pResult, eye);
        gsl_vector_scale(pResult, 1 / gsl_blas_dnrm2(pResult) * cameraSpeed);
        gsl_vector_sub(eye, pResult);
    }

    if (keys[0x41])
    {
        gsl_vector_memcpy(pResult, target);
        gsl_vector_sub(pResult, eye);
        vector_cross_product3(pResult, up, xAxis);
        gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis) * (-cameraSpeed));
        gsl_vector_add(eye, xAxis);
        gsl_vector_add(target, xAxis);
    }

    if (keys[0x44])
    {
        gsl_vector_memcpy(pResult, target);
        gsl_vector_sub(pResult, eye);
        vector_cross_product3(pResult, up, xAxis);
        gsl_vector_scale(xAxis, 1.0 / gsl_blas_dnrm2(xAxis) * (cameraSpeed));
        gsl_vector_add(eye, xAxis);
        gsl_vector_add(target, xAxis);
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
            ApplyTransformations();
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
            
            // for fps label
            rcModelInfo.left = rcClient.left;
            rcModelInfo.top = rcClient.bottom - 30;
            rcModelInfo.bottom = rcClient.bottom;
            rcModelInfo.right = rcClient.left + 80;

            // reinitializing BITMAPINFO structure
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
            zBuffer = realloc(zBuffer, bmp.bmHeight * bmp.bmWidth * sizeof(double));
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

                angleThetha -= (yMousePos - ptMousePrev.y) / 720.0 * 2 * M_PI;
                anglePhi += (xMousePos - ptMousePrev.x) / 1280.0 * 2 * M_PI;            

                gsl_vector_set_zero(target);
                eye->data[0] = destR * sin(angleThetha) * cos(anglePhi);
                eye->data[1] = destR * cos(angleThetha); 
                eye->data[2] = destR * sin(angleThetha) * sin(anglePhi);
                // ApplyMatrix(eye, MT_Y_ROTATE, 0, 0, 0, -(xMousePos - ptMousePrev.x) / 1280.0 * 2 * M_PI);
                // ApplyMatrix(eye, MT_X_ROTATE, 0, 0, 0, -(yMousePos - ptMousePrev.y) / 720.0 * 2 * M_PI);

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
            ExitProcess(0); //fix
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