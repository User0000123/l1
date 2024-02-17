#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

#include <math.h>

#include "MatrixTransformations.h"

double motionMatrix[] = 
{
    1, 0, 0, R,
    0, 1, 0, R,
    0, 0, 1, R,
    0, 0, 0, 1
};

double scaleMatrix[] = 
{
    R, 0, 0, 0,
    0, R, 0, 0,
    0, 0, R, 0,
    0, 0, 0, 1
};

double xRotateMatrix[] = 
{
    1, 0, 0, 0,
    0, R, R, 0,
    0, R, R, 0,
    0, 0, 0, 1
};

double yRotateMatrix[] = 
{
    R, 0, R, 0,
    0, 1, 0, 0,
    R, 0, R, 0,
    0, 0, 0, 1
};

double zRotateMatrix[] = 
{
    R, R, 0, 0,
    R, R, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

gsl_matrix mMotion;
gsl_matrix mScale;
gsl_matrix mXRotate;
gsl_matrix mYRotate;
gsl_matrix mZRotate;

gsl_vector *result;
gsl_matrix *resultM;

void InitializeMatrixTrans()
{
    mMotion = gsl_matrix_view_array(motionMatrix, 4, 4).matrix;
    mScale = gsl_matrix_view_array(scaleMatrix, 4, 4).matrix;
    mXRotate = gsl_matrix_view_array(xRotateMatrix, 4, 4).matrix;
    mYRotate = gsl_matrix_view_array(yRotateMatrix, 4, 4).matrix;
    mZRotate = gsl_matrix_view_array(zRotateMatrix, 4, 4).matrix;
    result = gsl_vector_calloc(4);
    resultM = gsl_matrix_alloc(4,4);
}

void FinalizeMatrixTrans()
{
    gsl_vector_free(result);
    gsl_matrix_free(resultM);
}

void ApplyMatrix(gsl_vector *target, MT_TYPE mtType, double x, double y, double z, double angle)
{
    switch (mtType)
    {
        case MT_MOTION:
            motionMatrix[3] = x;        
            motionMatrix[7] = y;        
            motionMatrix[11] = z;
        
            gsl_blas_dgemv(CblasNoTrans, 1.0, &mMotion, target, 0, result);
            
            break;
        case MT_SCALE:
            scaleMatrix[0] = x;
            scaleMatrix[5] = y;
            scaleMatrix[10] = z;

            gsl_blas_dgemv(CblasNoTrans, 1.0, &mScale, target, 0, result);
            break;
        case MT_X_ROTATE:
            xRotateMatrix[5] = cos(angle);
            xRotateMatrix[6] = -sin(angle);
            xRotateMatrix[9] = sin(angle);
            xRotateMatrix[10] = cos(angle);

            gsl_blas_dgemv(CblasNoTrans, 1.0, &mXRotate, target, 0, result);
            break;
        case MT_Y_ROTATE:
            yRotateMatrix[0] = cos(angle);
            yRotateMatrix[2] = sin(angle);
            yRotateMatrix[8] = -sin(angle);
            yRotateMatrix[10] = cos(angle);

            gsl_blas_dgemv(CblasNoTrans, 1.0, &mYRotate, target, 0, result);
            break;
        case MT_Z_ROTATE:
            // there may be MT_Z_ROTATE
            break;
    }

    gsl_vector_memcpy(target, result);
}

void ApplyMatrixM(gsl_matrix *target, MT_TYPE mtType, double x, double y, double z, double angle)
{
    switch (mtType)
    {
        case MT_MOTION:
            motionMatrix[3] = x;        
            motionMatrix[7] = y;        
            motionMatrix[11] = z;
        
            gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, target, &mMotion, 0, resultM);
            
            break;
        case MT_SCALE:
            scaleMatrix[0] = x;
            scaleMatrix[5] = y;
            scaleMatrix[10] = z;

            gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, target, &mScale, 0, resultM);
            break;
        case MT_X_ROTATE:
            xRotateMatrix[5] = cos(angle);
            xRotateMatrix[6] = -sin(angle);
            xRotateMatrix[9] = sin(angle);
            xRotateMatrix[10] = cos(angle);

            gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, target, &mXRotate, 0, resultM);
            break;
        case MT_Y_ROTATE:
            yRotateMatrix[0] = cos(angle);
            yRotateMatrix[2] = sin(angle);
            yRotateMatrix[8] = -sin(angle);
            yRotateMatrix[10] = cos(angle);

            gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, target, &mYRotate, 0, resultM);
            break;
        case MT_Z_ROTATE:
            // there may be MT_Z_ROTATE
            break;
    }

    gsl_matrix_memcpy(target, resultM);
}

void MatrixMult(gsl_matrix *pM1, gsl_matrix *pM2)
{
    gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1.0, pM1, pM2, 0, resultM);

    gsl_matrix_memcpy(pM1, resultM);
}