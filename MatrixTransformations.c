#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>

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

void InitializeMatrixTrans()
{
    mMotion = gsl_matrix_view_array(motionMatrix, 4, 4).matrix;
    mScale = gsl_matrix_view_array(scaleMatrix, 4, 4).matrix;
    mXRotate = gsl_matrix_view_array(xRotateMatrix, 4, 4).matrix;
    mYRotate = gsl_matrix_view_array(yRotateMatrix, 4, 4).matrix;
    mZRotate = gsl_matrix_view_array(zRotateMatrix, 4, 4).matrix;
    result = gsl_vector_calloc(4);

    // gsl_matrix_transpose(&mMotion);
    // gsl_matrix_transpose(&mScale);
    // gsl_matrix_transpose(&mXRotate);
    // gsl_matrix_transpose(&mYRotate);
    // gsl_matrix_transpose(&mZRotate);
}

void FinalizeMatrixTrans()
{
    gsl_vector_free(result);
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
    }

    gsl_vector_memcpy(target, result);
}

// int main()
// {
//     InitializeMatrixTrans();
//     double mt[] = {1, 2, 3, 1};
//     gsl_vector matrix = gsl_vector_view_array(mt, 4).vector;

//     ApplyMatrix(&matrix, MT_MOTION, 0, 0, 0, 0);
    
// }