#include <stdio.h>
#include <stdlib.h>
#include "DArray.h"

PDynamicArray CreateArray(int initialSize)
{
    PDynamicArray array = NULL;

    if (initialSize <= 1)
    {
        printf("Initial size is less than 2.\n");
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

    array->nCurSize = 1;
    array->nMaxSize = initialSize;

    return array;
}

void CheckCapacity(PDynamicArray array)
{
    if (array == NULL)
    {
        return;
    }

    if (array->nCurSize + 1 >= array->nMaxSize)
    {
        array->nMaxSize += array->nMaxSize / 2;
        array->nMaxSize++;
        array->data = realloc(array->data, sizeof(void *) * array->nMaxSize);        
    }
}

void Add(PDynamicArray array, void* data)
{
    if (array == NULL)
    {
        return;
    }

    CheckCapacity(array);

    array->data[array->nCurSize++] = data;
}


void TrimToSize(PDynamicArray array)
{
    if (array == NULL)
    {
        return;
    }

    array->data = realloc(array->data, sizeof(void *) * array->nCurSize);
    array->nMaxSize = array->nCurSize;
}       

int DestroyArray(PDynamicArray array)
{
    if (array == NULL)
    {
        return 1;
    }

    for (long i = 0; i < array->nCurSize; i++)
    {
        free(array->data[i]);
    }

    free(array->data);
    return 0;
}