#define UNICODE
#define _UNICODE

#include <tchar.h>
#include <stdio.h>
#include <gsl/gsl_blas.h>

#include "Model.h"
#include "ModelResources.h"
#include "ModelSettings.h"
#include "MatrixTransformations.h"
#include "Parser.h"

// #include "DoubleBuffering.h"

#define PTH_OBJ_FILE  "C:\\Users\\Aleksej\\Downloads\\model\\model.obj"  

    /* Graphic window */
HWND hwndMainWindow;
HDC hdcBack;
HBITMAP hbmBack;
RECT rcClient;

    /* .OBJ file */
FILE *objFile;
ObjFile *pObjFile;

void InitializeResources()
{
    InitializeMatrixTrans();
    objFile = fopen(PTH_OBJ_FILE, "r");
    pObjFile = parseOBJ(objFile);    
    double width = 1270;
    double height = 720;
    double zNear = 1;
    double zFar = 4;

    double transl[] = 
    {
        2.0*zNear/width, 0, 0, 0, 
        0, 2.0 * zNear/height, 0, 0,
        0, 0, zFar / (zNear - zFar), zNear*zFar/(zNear - zFar),
        0, 0, -1, 0
    };
    gsl_matrix matrix = gsl_matrix_view_array(transl, 4, 4).matrix;
    gsl_vector *pResult = gsl_vector_calloc(4);

    for (int i = 1; i < pObjFile->v->nCurSize; i++)
    {
        // gsl_vector *pVector = pObjFile->v->data[i];
        // gsl_vector_printf("")
        // gsl_blas_dgemv(CblasNoTrans, 1.0, &matrix, pObjFile->v->data[i], 0, pVector);
        // gsl_vector_memcpy(pVector, pResult);
        
        // for (int j = 0; j < 3; j++)
        // {
        //     pVector->data[j] /= pVector->data[3];
        // }

        ApplyMatrix(pObjFile->v->data[i], MT_MOTION, 1270/2, 360 / 2 + 200, 0, 0);
        ApplyMatrix(pObjFile->v->data[i], MT_SCALE, 2, 1, 0, 0);
        // ApplyMatrix(pObjFile->v->data[i], MT_Z_ROTATE, 0, 0, 0, 180);
    }
}

void FreeAllResources()
{
    FinalizeMatrixTrans();
    DestroyObjFileInfo(pObjFile);
    fclose(objFile);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        /* Section for window messages */
        case WM_CREATE:
            InitializeResources();
            break;
        case WM_PAINT:
            HDC dc;
            PAINTSTRUCT ps;

            dc = BeginPaint(hWnd, &ps);

            for (int i = 1; i < pObjFile->v->nCurSize; i++)
            {
                gsl_vector *pVector = pObjFile->v->data[i];

                double x = gsl_vector_get(pVector, 0);
                double y = gsl_vector_get(pVector, 1);

                Ellipse(dc, x, y, x+2, y+2);                
            }

            EndPaint(hWnd, &ps);
            break;
        case WM_SIZE:
            // FinalizeBuffer(&hdcBack, &hbmBack);
            // InitializeBuffer(hWnd, &hdcBack, &hbmBack, &rcClient);
            break;
        case WM_LBUTTONDOWN:
        case WM_KEYDOWN:
            break;
        case WM_DESTROY:
            FreeAllResources();
            // FinalizeBuffer(&hdcBack, &hbmBack);
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