#ifndef __DARRAY_H__
#define __DARRAY_H__

typedef struct 
{
    void** data;
    long nCurSize;
    long nMaxSize;
} TDynamicArray, *PDynamicArray;

PDynamicArray   CreateArray(int initialSize);
void            Add(PDynamicArray array, void* data);
void            TrimToSize(PDynamicArray array);             
int             DestroyArray(PDynamicArray array);
void            CleanArray(PDynamicArray array);

#endif