#include <gsl/gsl_matrix.h>
#include <stdlib.h>

#include "DArray.h"

int main()
{
    PDynamicArray array = CreateArray(2);
    Add(array, (void *)1);
    Add(array, (void *)2);
    Add(array, (void *)3);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);
    Add(array, (void *)4);


    for (int i = 1; i < array->nCurSize; i++)
    {
        printf("%d ", array->data[i]);
    }

    printf("\nSize: %d", array->nCurSize);
    printf("\nSize: %d", array->nMaxSize);
    return 0;
}

// void *parceVerteces(FILE *stream)
// {
    
// }

// void *parcePolygons(FILE *stream)
// {

// }