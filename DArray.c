#include <stdio.h>
#include <stdlib.h>
#include "DArray.h"

PDynamicArray CreateArray(int initialSize)
{
    PDynamicArray array = NULL;

    if (initialSize < 0)
    {
        printf("Initial size is less than 0.\n");
        return NULL;
    }

    array = malloc(sizeof(TDynamicArray));
    if (array == NULL)
    {
        printf("There is insufficient memory available.\n");
        return NULL;
    }

    array->data = malloc(sizeof(void *) * initialSize);
    if (array->data == NULL)
    {
        printf("There is insufficient memory available.\n");
        return NULL;
    }

    array->nCurSize = 0;
    array->nMaxSize = initialSize;

    return array;
}

void CheckCapacity(PDynamicArray array)
{
    if (array->nCurSize == array->nMaxSize)
}
void            TrimToSize(PDynamicArray array);             
int             DestroyArray(PDynamicArray array);