#ifndef __DARRAY_H__
#define __DARRAY_H__

typedef struct 
{
    void* data;
    long nCurSize;
    long nMaxSize;
} TDynamicArray, *PDynamicArray;

PDynamicArray   CreateArray(int initialSize);
void            CheckCapacity(PDynamicArray array);
void            TrimToSize(PDynamicArray array);             
int             DestroyArray(PDynamicArray array);

#endif