#define UNICODE
#define _UNICODE

#include <tchar.h>
#include "BlockingQueue.h"

typedef struct SQELEMENT{
    struct SQELEMENT* next;
    LPVOID data;
} TQELEMENT, *PTQELEMENT;

typedef struct {
    PTQELEMENT head;
    PTQELEMENT tail;
    int queueMaxLength;
    int queueCurrentLength;
    CRITICAL_SECTION csBufferLock;
    CONDITION_VARIABLE cvBufferNotFull;
    CONDITION_VARIABLE cvBufferNotEmpty;
} TQUEUE, *PQUEUE;

void enqueue(PTQELEMENT pQElement, PQUEUE pQueue)
{
    if (pQueue->head == NULL)
    {
        pQueue->head = pQElement;
    } 
    else 
    {
        pQueue->tail->next = pQElement;
    }

    pQueue->tail = pQElement;
    pQueue->queueCurrentLength++;
}

LPVOID dequeue(PQUEUE pQueue)
{
    LPVOID data = pQueue->head->data;
    PTQELEMENT temp = pQueue->head;

    data = pQueue->head->data;
    pQueue->head = pQueue->head->next;

    if (pQueue->head == NULL)
    {
        pQueue->tail = NULL;
    }

    if (HeapFree(GetProcessHeap(), 0, temp) == FALSE)
    {
        _tprintf(_TEXT("Невозможно освободить память в куче"));
    }

    pQueue->queueCurrentLength--;
    return data;
}

HBLOCKQUEUE CreateQueue(int nQueueMaxElements)
{
    PQUEUE pQueue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TQUEUE));
    
    if (pQueue != NULL) 
    {
        pQueue->queueMaxLength = nQueueMaxElements;
        InitializeCriticalSection(&pQueue->csBufferLock);
        InitializeConditionVariable(&pQueue->cvBufferNotFull);
        InitializeConditionVariable(&pQueue->cvBufferNotEmpty);
    }
    else 
    {
        _tprintf(_TEXT("Невозможно выделить память в куче"));
    }

    return (HBLOCKQUEUE) pQueue;
}

BOOL DestroyQueue(HBLOCKQUEUE hConQueue)
{
    PQUEUE pQueue = (PQUEUE) hConQueue;

    
    for (;pQueue->queueCurrentLength != 0 && Remove(hConQueue, NULL););
    
    DeleteCriticalSection(&pQueue->csBufferLock);
    return HeapFree(GetProcessHeap(), 0, (PQUEUE) hConQueue);
}

BOOL Append(HBLOCKQUEUE hConQueue, LPVOID data)
{
    PQUEUE pQueue = (PQUEUE) hConQueue;
    PTQELEMENT pQElement = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TQELEMENT));
    BOOL state = FALSE;

    if (pQElement != NULL)
    {
        pQElement->data = data;

        EnterCriticalSection(&pQueue->csBufferLock);
         
        while (pQueue->queueCurrentLength == pQueue->queueMaxLength)
        {
            SleepConditionVariableCS(&pQueue->cvBufferNotFull, &pQueue->csBufferLock, 100);
        }
        
        enqueue(pQElement, pQueue);
        state = TRUE;

        LeaveCriticalSection(&pQueue->csBufferLock);

        WakeConditionVariable(&pQueue->cvBufferNotEmpty);
    }
    else 
    {
        _tprintf(_TEXT("Невозможно выделить память в куче"));
    }

    return state;
}

BOOL Remove(HBLOCKQUEUE hConQueue, LPVOID* buffer)
{
    PQUEUE pQueue = (PQUEUE) hConQueue;

    EnterCriticalSection(&pQueue->csBufferLock);

    while (pQueue->queueCurrentLength == 0) 
    {
        SleepConditionVariableCS(&pQueue->cvBufferNotFull, &pQueue->csBufferLock, 100);
    } 

    if (buffer != NULL) 
    {
        (*buffer) = dequeue(pQueue);
    }

    LeaveCriticalSection(&pQueue->csBufferLock);
    WakeConditionVariable(&pQueue->cvBufferNotFull);
    
    return TRUE;
}