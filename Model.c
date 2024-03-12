#define UNICODE
#define _UNICODE
#define _WIN32_WINNT_WIN10

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
//#define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\model\\model.obj" 
// #define PTH_OBJ_FILE        "C:\\test.obj" 
// #define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\the-billiards-room\\source\\{E92F06F9-2FE5-440C-80A3-14D7B6C23206}\\model.obj" 
// #define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\capybara(1)\\capybara.obj" 
// #define PTH_OBJ_FILE        "C:\\Users\\Aleksej\\Downloads\\carpincho-capybara-vrchat-avatar\\source\\Carpincho\\Carpincho.obj"
 #define PTH_OBJ_FILE        "Model\\model.obj"

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
#define Z_FAR               5

    /* Graphic window */
HWND hwndMainWindow;
RECT rcClient;
WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };

    /* Back buffer */
HDC hdcBack;
HBITMAP hbmBack;

    /* Screen bitmap */
BITMAPINFO bmi;
HBRUSH hbrBackground;
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
DOUBLE angleThetha =    M_PI_2;
DOUBLE anglePhi =       M_PI_2;

    /* Light */
gsl_vector *light;

    /* Z buffer */
double *zBuffer;
CRITICAL_SECTION *zBufferCS;
byte *pIsDrawable;

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
#define N_QUEUE_MAX_SIZE    10000
#define N_THREADS           30
#define N_PARAMS            6
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
		gsl_vector *pVN0;
		gsl_vector *pVN1;
		gsl_vector *pVN2;
		gsl_vector *pAN;
    gsl_vector *pBN;
    gsl_vector *pPN;
} PARAMS;
PARAMS params[N_PARAMS]; // <- There can be any size you want
HANDLE hTaskExecutedEvent;

PTP_POOL pThreadPool;
TP_CALLBACK_ENVIRON tpCBEnvironment;
PTP_CLEANUP_GROUP tpCUGroup;
volatile LONG count;

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
        0, 0, (double) Z_FAR / (Z_NEAR - Z_FAR), (double) Z_NEAR * Z_FAR / (Z_NEAR - Z_FAR),
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

inline int isInside(gsl_vector *pPoint)
{
    return ((pPoint->data[0] > -pPoint->data[3] && pPoint->data[0] < pPoint->data[3]) && 
        (pPoint->data[1] > -pPoint->data[3] && pPoint->data[1] < pPoint->data[3]) &&
        (pPoint->data[2] > 0 && pPoint->data[2] < pPoint->data[3]));
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
    // gsl_matrix_memcpy(matrixTransformation, &matrixViewPort);
    gsl_matrix_memcpy(matrixTransformation, &matrixProjection);

    // MatrixMult(matrixTransformation, &matrixProjection);
    MatrixMult(matrixTransformation, &matrixView);
    // Also, there may be multiplication by the tranformation matix 
    ////////
    
    for (int i = 1; i < pObjFile->v->nCurSize; i++)
    {
        gsl_vector *pVector = gvPaintVertices[i];

        gsl_blas_dgemv(CblasNoTrans, 1.0, matrixTransformation, pVector, 0, pResult);
        
        if (pIsDrawable[i] = isInside(pResult))
        {
            gsl_vector_memcpy(pVector, pResult);
            gsl_blas_dgemv(CblasNoTrans, 1.0, &matrixViewPort, pVector, 0, pResult);
            gsl_vector_memcpy(pVector, pResult);
            gsl_vector_scale(pVector, 1.0 / gsl_vector_get(pVector, 3));
        }
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
    
    pIsDrawable = calloc(pObjFile->v->nCurSize + 1, sizeof(byte));

    ApplyTransformations();

    tcsFpsInfo = calloc(FPS_OUT_MAX_LENGTH, sizeof(TCHAR));

    // hPool = CreateThreadPool(N_QUEUE_MAX_SIZE, N_THREADS);
    // hTaskExecutedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    pThreadPool = CreateThreadpool(NULL);
    SetThreadpoolThreadMaximum(pThreadPool, 500);
    SetThreadpoolThreadMinimum(pThreadPool, N_THREADS);
    InitializeThreadpoolEnvironment(&tpCBEnvironment);
    tpCUGroup = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&tpCBEnvironment, tpCUGroup, NULL);

    light = gsl_vector_calloc(4);

    for (int i = 0; i < N_PARAMS; i++)
    {
        params[i].pV0 = gsl_vector_calloc(4);
        params[i].pV1 = gsl_vector_calloc(4); 
        params[i].pV2 = gsl_vector_calloc(4);
        params[i].pA = gsl_vector_calloc(4);
        params[i].pB = gsl_vector_calloc(4);
        params[i].pP = gsl_vector_calloc(4);
        params[i].norm = gsl_vector_calloc(4);
				params[i].pVN0 = gsl_vector_calloc(4);
        params[i].pVN1 = gsl_vector_calloc(4); 
        params[i].pVN2 = gsl_vector_calloc(4);
				params[i].pAN = gsl_vector_alloc(4);
				params[i].pBN = gsl_vector_alloc(4);
				params[i].pPN = gsl_vector_alloc(4);
    }

    hbrBackground = CreateSolidBrush(RGB(255, 0, 0));
}

void FreeAllResources()
{
    CloseHandle(hTaskExecutedEvent);
    free(tcsFpsInfo);
    free(gvPaintVertices);
    free(gvWorldVertices);

    gsl_block_free(gbPaintVertices);
    gsl_block_free(gbWorldVertices);
    gsl_matrix_free(matrixTransformation);
    gsl_vector_free(light);

    for (int i = 0; i < N_PARAMS; i++)
    {
        gsl_vector_free(params[i].pA);
        gsl_vector_free(params[i].pB);
        gsl_vector_free(params[i].pV0);
        gsl_vector_free(params[i].pV1);
        gsl_vector_free(params[i].pV2);
        gsl_vector_free(params[i].pP);
        gsl_vector_free(params[i].norm);
    }

    free(pResult);
    free(eye);
    free(target);
    free(up);
    free(xAxis);
    free(yAxis);
    free(zAxis);

    free(pBytes);
    free(zBuffer);
    free(pIsDrawable);

    DestroyObjFileInfo(pObjFile);
    fclose(objFile);
    FinalizeMatrixTrans();
    KillTimer(hwndMainWindow, TIMER_REPAINT_ID);
    
    DeleteObject(hbrBackground);
}

inline void DrawLine(HDC dc, int x0, int y0, int x1, int y1, byte r, byte g, byte b)
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
        pBytes[offset + 0] = r;
        pBytes[offset + 1] = g;
        pBytes[offset + 2] = b;
        // InterlockedOr((long *)(pBytes + offset), 0x00FFFFFF);

        error += derror;

        if (error > dx)
        {
            y += signY;
            error -= dxDouble;
        }
    }
}

/**
 * @brief 
 * 
 * @param pThreadParams 
 * @param dc 
 * @param pTriangleVertices Sequence numbers of vertexes in vertexes coordinates array from .obj @see gvPaintVertices
 * @param r 
 * @param g 
 * @param b 
 */
inline void DrawTriangle(PARAMS *pThreadParams, HDC dc, gsl_vector_int *pTriangleVertices, byte r, byte g, byte b)
{
    if (!(pIsDrawable[pTriangleVertices->data[0]] && pIsDrawable[pTriangleVertices->data[1]] && pIsDrawable[pTriangleVertices->data[2]])) return;

    gsl_vector_memcpy(pThreadParams->pV0, gvPaintVertices[pTriangleVertices->data[0]]);
    gsl_vector_memcpy(pThreadParams->pV1, gvPaintVertices[pTriangleVertices->data[1]]);
    gsl_vector_memcpy(pThreadParams->pV2, gvPaintVertices[pTriangleVertices->data[2]]);

    if (pThreadParams->pV0->data[1] > pThreadParams->pV1->data[1]) SWAP_VECTORS(pThreadParams->pV0, pThreadParams->pV1); //<!-- gsl_vactor_swap exists 
    if (pThreadParams->pV0->data[1] > pThreadParams->pV2->data[1]) SWAP_VECTORS(pThreadParams->pV0, pThreadParams->pV2);
    if (pThreadParams->pV1->data[1] > pThreadParams->pV2->data[1]) SWAP_VECTORS(pThreadParams->pV1, pThreadParams->pV2);

    for (int i = 0; i < 2; i++)
    {
        pThreadParams->pV0->data[i] = round(pThreadParams->pV0->data[i]);
        pThreadParams->pV1->data[i] = round(pThreadParams->pV1->data[i]);
        pThreadParams->pV2->data[i] = round(pThreadParams->pV2->data[i]);
    }
    if (abs(pThreadParams->pV0->data[1] - pThreadParams->pV1->data[1]) < 1 && abs(pThreadParams->pV0->data[1] - pThreadParams->pV2->data[1]) < 1) return; //<!-- realy need this? it is validation of .obj file actually

    int total_height = pThreadParams->pV2->data[1] - pThreadParams->pV0->data[1];

    for (int i=0; i<total_height; i++) {
        BOOL second_half = i > pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] || abs(pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]) < 1;
        int segment_height = (second_half ? (pThreadParams->pV2->data[1] - pThreadParams->pV1->data[1]) : (pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]));
        double alpha = (double) i / total_height;
        double beta  = (double) (i - (second_half ? pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] : 0)) / segment_height;
        
        gsl_vector_memcpy(pThreadParams->pA, pThreadParams->pV2);
        gsl_vector_sub(pThreadParams->pA, pThreadParams->pV0);
        gsl_vector_scale(pThreadParams->pA, alpha);
        gsl_vector_add(pThreadParams->pA, pThreadParams->pV0);    

        if (second_half)
        {
            gsl_vector_memcpy(pThreadParams->pB, pThreadParams->pV2);
            gsl_vector_sub(pThreadParams->pB, pThreadParams->pV1);
            gsl_vector_scale(pThreadParams->pB, beta);
            gsl_vector_add(pThreadParams->pB, pThreadParams->pV1);
        }
        else
        {
            gsl_vector_memcpy(pThreadParams->pB, pThreadParams->pV1);
            gsl_vector_sub(pThreadParams->pB, pThreadParams->pV0);
            gsl_vector_scale(pThreadParams->pB, beta);
            gsl_vector_add(pThreadParams->pB, pThreadParams->pV0);
        }
        
        pThreadParams->pA->data[0] = round(pThreadParams->pA->data[0]);
        pThreadParams->pA->data[1] = round(pThreadParams->pA->data[1]);
        pThreadParams->pB->data[0] = round(pThreadParams->pB->data[0]);
        pThreadParams->pB->data[1] = round(pThreadParams->pB->data[1]);

        int offset;
        if (pThreadParams->pA->data[0] > pThreadParams->pB->data[0]) SWAP_VECTORS(pThreadParams->pA, pThreadParams->pB);
        // pThreadParams->pA->data[0] += 1;
        for (int j = pThreadParams->pA->data[0]+1; j <= pThreadParams->pB->data[0]; j++) {
            if (j < 0 || j >= bmp.bmWidth || ((int) pThreadParams->pV0->data[1] + i) < 0 || ((int) pThreadParams->pV0->data[1] + i) >= bmp.bmHeight) continue;
            double phi = abs(pThreadParams->pB->data[0] - pThreadParams->pA->data[0]) < 1 ? 1.0 : (double)(j-pThreadParams->pA->data[0])/(double)(pThreadParams->pB->data[0]-pThreadParams->pA->data[0]+1);
            gsl_vector_memcpy(pThreadParams->pP, pThreadParams->pB);
            gsl_vector_sub(pThreadParams->pP, pThreadParams->pA);
            gsl_vector_scale(pThreadParams->pP, phi);
            gsl_vector_add(pThreadParams->pP, pThreadParams->pA);

            offset = (((int) pThreadParams->pV0->data[1] + i) * bmp.bmWidth + j) << 2;
            int idx = j+(pThreadParams->pV0->data[1] + i)*bmp.bmWidth;
            
            EnterCriticalSection(zBufferCS + idx);
            if (zBuffer[idx] > pThreadParams->pP->data[2]) {
                zBuffer[idx] = pThreadParams->pP->data[2];

                pBytes[offset + 0] = r;
                pBytes[offset + 1] = g;
                pBytes[offset + 2] = b;   
            }
            LeaveCriticalSection(zBufferCS+idx);
        }
    }
}

void calculateNormal(gsl_vector* first, gsl_vector* second, double scale, gsl_vector* result) {
		gsl_vector_memcpy(result, second);
		gsl_vector_sub(result, first);
		gsl_vector_scale(result, scale);
		gsl_vector_add(result, first);
		gsl_vector_scale(result, 1.0 / gsl_blas_dnrm2(result));
}

void calculateSideScanlinePoints(PARAMS *pThreadParams, double alpha, BOOL second_half, double beta) {
		gsl_vector_memcpy(pThreadParams->pA, pThreadParams->pV2);
		gsl_vector_sub(pThreadParams->pA, pThreadParams->pV0);
		gsl_vector_scale(pThreadParams->pA, alpha);
		gsl_vector_add(pThreadParams->pA, pThreadParams->pV0);

		calculateNormal(pThreadParams->pVN0, pThreadParams->pVN2, alpha, pThreadParams->pAN);

		if (second_half)
		{
				gsl_vector_memcpy(pThreadParams->pB, pThreadParams->pV2);
				gsl_vector_sub(pThreadParams->pB, pThreadParams->pV1);
				gsl_vector_scale(pThreadParams->pB, beta);
				gsl_vector_add(pThreadParams->pB, pThreadParams->pV1);
				calculateNormal(pThreadParams->pVN1, pThreadParams->pVN2, beta, pThreadParams->pBN);
		}
		else
		{
				gsl_vector_memcpy(pThreadParams->pB, pThreadParams->pV1);
				gsl_vector_sub(pThreadParams->pB, pThreadParams->pV0);
				gsl_vector_scale(pThreadParams->pB, beta);
				gsl_vector_add(pThreadParams->pB, pThreadParams->pV0);
				calculateNormal(pThreadParams->pVN0, pThreadParams->pVN1, beta, pThreadParams->pBN);
		}
}

inline void DrawTriangleIl(PARAMS *pThreadParams, HDC dc, int index)
{
		gsl_vector_int *pTriangleVertices = pObjFile->fv->data[index];
    if (!(pIsDrawable[pTriangleVertices->data[0]] && pIsDrawable[pTriangleVertices->data[1]] && pIsDrawable[pTriangleVertices->data[2]])) return;

		/*CHANGES*/
		gsl_vector_memcpy(pThreadParams->pVN0, pObjFile->vn->data[pTriangleVertices->data[0]]); //<!-- can be moved into resource initialization ferther
		gsl_vector_memcpy(pThreadParams->pVN1, pObjFile->vn->data[pTriangleVertices->data[1]]); //<!-- can be moved into resource initialization ferther
		gsl_vector_memcpy(pThreadParams->pVN2, pObjFile->vn->data[pTriangleVertices->data[2]]); //<!-- can be moved into resource initialization ferther

    gsl_vector_memcpy(pThreadParams->pV0, gvPaintVertices[pTriangleVertices->data[0]]);
    gsl_vector_memcpy(pThreadParams->pV1, gvPaintVertices[pTriangleVertices->data[1]]);
    gsl_vector_memcpy(pThreadParams->pV2, gvPaintVertices[pTriangleVertices->data[2]]);

    if (pThreadParams->pV0->data[1] > pThreadParams->pV1->data[1]) {
			SWAP_VECTORS(pThreadParams->pV0, pThreadParams->pV1); //<!-- gsl_vactor_swap exists
			SWAP_VECTORS(pThreadParams->pVN0, pThreadParams->pVN1);
		}
    if (pThreadParams->pV0->data[1] > pThreadParams->pV2->data[1]) {
			SWAP_VECTORS(pThreadParams->pV0, pThreadParams->pV2);
			SWAP_VECTORS(pThreadParams->pVN0, pThreadParams->pVN2);
		}
    if (pThreadParams->pV1->data[1] > pThreadParams->pV2->data[1]) {
			SWAP_VECTORS(pThreadParams->pV1, pThreadParams->pV2);
			SWAP_VECTORS(pThreadParams->pVN1, pThreadParams->pVN2);
		}

    for (int i = 0; i < 2; i++)
    {
        pThreadParams->pV0->data[i] = round(pThreadParams->pV0->data[i]);
        pThreadParams->pV1->data[i] = round(pThreadParams->pV1->data[i]);
        pThreadParams->pV2->data[i] = round(pThreadParams->pV2->data[i]);
    }
    if (abs(pThreadParams->pV0->data[1] - pThreadParams->pV1->data[1]) < 1 && abs(pThreadParams->pV0->data[1] - pThreadParams->pV2->data[1]) < 1) return; //<!-- realy need this? it is validation of .obj file actually

    int total_height = pThreadParams->pV2->data[1] - pThreadParams->pV0->data[1];

    for (int i=0; i<total_height; i++) {
        BOOL second_half = i > pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] || abs(pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]) < 1;
        int segment_height = (second_half ? (pThreadParams->pV2->data[1] - pThreadParams->pV1->data[1]) : (pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1]));
        double alpha = (double) i / total_height;
        double beta  = (double) (i - (second_half ? pThreadParams->pV1->data[1] - pThreadParams->pV0->data[1] : 0)) / segment_height;
        
				calculateSideScanlinePoints(pThreadParams, alpha, second_half, beta);

				for (int i = 0; i < 2; i++)
				{
						pThreadParams->pA->data[i] = round(pThreadParams->pA->data[i]);
						pThreadParams->pB->data[i] = round(pThreadParams->pB->data[i]);
				}

        if (pThreadParams->pA->data[0] > pThreadParams->pB->data[0]) {
					 SWAP_VECTORS(pThreadParams->pA, pThreadParams->pB);
					 SWAP_VECTORS(pThreadParams->pAN, pThreadParams->pBN);
				}

				int offset;

								gsl_vector* diff = gsl_vector_alloc(4);
								gsl_vector *reflection = gsl_vector_alloc(4);
								gsl_vector *Rr = gsl_vector_alloc(4);
								gsl_vector *V = gsl_vector_alloc(4);

        for (int j = pThreadParams->pA->data[0]+1; j <= pThreadParams->pB->data[0]; j++) {
            if (j < 0 || j >= bmp.bmWidth || ((int) pThreadParams->pV0->data[1] + i) < 0 || ((int) pThreadParams->pV0->data[1] + i) >= bmp.bmHeight) continue;
            double phi = abs(pThreadParams->pB->data[0] - pThreadParams->pA->data[0]) < 1 ? 1.0 : (double)(j-pThreadParams->pA->data[0])/(double)(pThreadParams->pB->data[0]-pThreadParams->pA->data[0]+1);
            gsl_vector_memcpy(pThreadParams->pP, pThreadParams->pB);
            gsl_vector_sub(pThreadParams->pP, pThreadParams->pA);
            gsl_vector_scale(pThreadParams->pP, phi);
            gsl_vector_add(pThreadParams->pP, pThreadParams->pA);

						calculateNormal(pThreadParams->pAN, pThreadParams->pBN, phi, pThreadParams->pPN);

            offset = (((int) pThreadParams->pV0->data[1] + i) * bmp.bmWidth + j) << 2;
            int idx = j+(pThreadParams->pV0->data[1] + i)*bmp.bmWidth;
            
            EnterCriticalSection(zBufferCS + idx);
            if (zBuffer[idx] > pThreadParams->pP->data[2]) {
                zBuffer[idx] = pThreadParams->pP->data[2];

								double shininessVal = 80.0;
								//gsl_vector *L = gsl_vector_alloc(4);
								

								gsl_vector *L = gsl_vector_alloc(4);
								gsl_vector_memcpy(L, eye);
								gsl_vector_scale(L, 1.0 / gsl_blas_dnrm2(L));

								double lambertian;
								gsl_blas_ddot(pThreadParams->pPN, L, &lambertian);
								lambertian = max(0, lambertian);

								// gsl_vector_memcpy(L, eye);
								// gsl_vector_sub(L, pThreadParams->pP);
  							// gsl_vector_scale(L, 1.0 / gsl_blas_dnrm2(L));

								double specular = 0.0;

								if(lambertian > 0.0) {
									
									gsl_vector_memcpy(diff, L);
									gsl_vector_sub(diff, pThreadParams->pPN);

									double tmp;
									gsl_blas_ddot(diff, pThreadParams->pPN, &tmp);

									
									gsl_vector_memcpy(reflection, pThreadParams->pPN);
									gsl_vector_scale(reflection, 2 * tmp);
									gsl_vector_sub(reflection, diff);

									
									gsl_vector_memcpy(Rr, pThreadParams->pPN);
									gsl_vector_add(Rr, reflection);

									
									gsl_vector_memcpy(V, pThreadParams->pP);
									gsl_vector_scale(V, 1.0 / gsl_blas_dnrm2(V));
									// Compute the specular term
									
									double specAngle;
									gsl_blas_ddot(Rr, V, &specAngle);
									specAngle = max(specAngle, 0.0);
									specular = pow(specAngle, shininessVal);

									
								}

								gsl_vector_free(L);

                pBytes[offset + 0] = ((lambertian * 204) + 52 + (specular * 255)); 
                pBytes[offset + 1] = ((lambertian * 102) + 25 + (specular * 255));
                pBytes[offset + 2] = ((lambertian * 0) + 0 + (specular * 255));   
            }
            LeaveCriticalSection(zBufferCS+idx);
        }

					gsl_vector_free(V);
								gsl_vector_free(Rr);
								gsl_vector_free(reflection);
								gsl_vector_free(diff);
    }
}

void flatShading(int index, PARAMS *param, double *intens)
{
    double global_intens = 0;
    double result;
    gsl_vector_int *pVector = pObjFile->fvn->data[index];

    for (int i = 0; i < 3; i++)
    {
        gsl_vector_memcpy(param->pV0, pObjFile->vn->data[pVector->data[i]]);
        gsl_blas_ddot(param->pV0, light, &result);

        global_intens -= result;
    }

    *intens = global_intens / 3.0;
}

void CALLBACK task(PTP_CALLBACK_INSTANCE Instance, PVOID params, PTP_WORK Work)
{
    PARAMS *param = (PARAMS *) params;
    for (int i = param->from; i < param->to; i++)
    {
        DrawTriangleIl(param, param->dc, i);
    }
    
    InterlockedAdd(&count, 1);
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
            
    FillRect(hdc, &rcClient, hbrBackground);

    GetDIBits(hdc, hbmBack, 0, bmp.bmHeight, pBytes, &bmi, DIB_RGB_COLORS);
    
    for (int i = 0; i < bmp.bmWidth * bmp.bmHeight; i++)
    {
        zBuffer[i] = 100;
    }
    gsl_vector_memcpy(light, target);
    gsl_vector_sub(light, eye);
    gsl_vector_scale(light, 1.0 / gsl_blas_dnrm2(light));

    PTP_WORK work;
    int delta = pObjFile->fv->nCurSize / N_PARAMS;
    params[0].from = 1;
    params[0].to = delta;
    params[0].dc = hdc;

    // SubmitTask(hPool, task, params);
    work = CreateThreadpoolWork(task, params, &tpCBEnvironment);
    SubmitThreadpoolWork(work);
    for (int i = 1; i < N_PARAMS - 1; i++)
    {
        params[i].from = params[i - 1].to;
        params[i].to = params[i].from + delta;
        params[i].dc = hdc;
        work = CreateThreadpoolWork(task, params + i, &tpCBEnvironment);
        SubmitThreadpoolWork(work);
        // SubmitTask(hPool, task, params + i);
    }
    params[N_PARAMS - 1].from = params[N_PARAMS - 2].to;
    params[N_PARAMS - 1].to = pObjFile->fv->nCurSize;
    params[N_PARAMS - 1].dc = hdc;
    work = CreateThreadpoolWork(task, params + N_PARAMS - 1, &tpCBEnvironment);
    SubmitThreadpoolWork(work);
    // SubmitTask(hPool, task, params + N_PARAMS - 1);
    
    // waiting for all threads finish drawing
    while (count < N_PARAMS)
    {
    }
    count = 0;
    CloseThreadpoolCleanupGroupMembers(tpCUGroup,
                                       FALSE,
                                       &tpCBEnvironment);

    
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
            // InvalidateRect(hWnd, NULL, FALSE);
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
            zBufferCS = realloc(zBufferCS, bmp.bmHeight * bmp.bmWidth * sizeof(CRITICAL_SECTION));
            for (int i = 0; i < bmp.bmHeight * bmp.bmWidth; i++)
            {
                InitializeCriticalSectionAndSpinCount(zBufferCS + i, 2000);
            }
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
            ExitProcess(0); //need some fix
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
    
    // while (GetMessage(&msg, NULL, 0, 0))
    // {
    //     DispatchMessage(&msg);
    //     fpsLOG();
    // }

    for (;;)
    {
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&msg);
        }
        fpsLOG();
        InvalidateRect(hwndMainWindow, NULL, FALSE);
    }

    return (int) msg.wParam;
}