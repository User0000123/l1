#ifndef __MT_H__
#define __MT_H__

#include <gsl/gsl_matrix.h>

#define R     999.0

typedef enum 
{   
    MT_MOTION, MT_SCALE, MT_X_ROTATE, MT_Y_ROTATE, MT_Z_ROTATE    
} MT_TYPE;

void InitializeMatrixTrans();
void FinalizeMatrixTrans();
void ApplyMatrix(gsl_vector *target, MT_TYPE mtType, double x, double y, double z, double angle);
void ApplyMatrixM(gsl_matrix *target, MT_TYPE mtType, double x, double y, double z, double angle);
void MatrixMult(gsl_matrix *pM1, gsl_matrix *pM2);

#endif