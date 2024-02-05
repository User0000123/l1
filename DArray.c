#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    array->data = calloc(initialSize, sizeof(void *));
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

    if (array->nCurSize + 1 > array->nMaxSize)
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

void CleanArray(PDynamicArray array)
{
    if (array == NULL)
    {
        return;
    }

    for (long i = 0; i < array->nCurSize; i++)
    {
        free(array->data[i]);
    }

    array->nCurSize = 1;
}

int DestroyArray(PDynamicArray array)
{
    CleanArray(array);

    free(array->data);
    return 0;
}