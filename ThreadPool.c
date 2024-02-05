#define UNICODE
#define _UNICODE

#include <tchar.h>
#include "ThreadPool.h"
#include "BlockingQueue.h"

typedef struct {
    HANDLE* hThreads;
    HBLOCKQUEUE hBlockingQueue;
    long nExecutedTasks;
    int nThreads;
} THDPOOL, *PTHDPOOL;

typedef struct {
    LPTASKFUNC runnableTask;
    LPVOID params;
} TASKELEMENT, *PTASKELEMENT;


DWORD WINAPI TaskAccepter(LPVOID lParam)
{
    PTHDPOOL pTDPool = (PTHDPOOL) lParam; 
    PTASKELEMENT pTaskElement;

    while (TRUE)
    {
        Remove(pTDPool->hBlockingQueue, (LPVOID)&pTaskElement);
        pTaskElement->runnableTask(pTaskElement->params);
        InterlockedAdd(&pTDPool->nExecutedTasks, 1);
    }

    return TRUE;
}

BOOL SubmitTask(HTHDPOOL hThPool, LPTASKFUNC runnableTask, LPVOID lParams)
{
    PTHDPOOL pTDPool = (PTHDPOOL) hThPool; 
    PTASKELEMENT taskElement = HeapAlloc(GetProcessHeap(), 0, sizeof(TASKELEMENT));
    BOOL state = FALSE;

    if (taskElement != NULL)
    {
        taskElement->params = lParams;
        taskElement->runnableTask = runnableTask;

        Append(pTDPool->hBlockingQueue, taskElement);
        state = TRUE;
    }
    else
    {
        _tprintf(_TEXT("Ошибка выделения памяти для задания!"));
    }

    return state;
}

HTHDPOOL CreateThreadPool(int nTasksMaxSize, int nThreadsMaxSize)
{
    PTHDPOOL pTDPool = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(THDPOOL));

    if (pTDPool != NULL)
    {
        pTDPool->hBlockingQueue = CreateQueue(nTasksMaxSize);
        
        if (pTDPool->hBlockingQueue != 0)
        {
            pTDPool->hThreads = HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE) * nThreadsMaxSize);

            if (pTDPool->hThreads != NULL)
            {
                for (int i = 0; i < nThreadsMaxSize; i++)
                {
                    pTDPool->hThreads[i] = CreateThread(NULL, 0, TaskAccepter, pTDPool, 0, NULL);
                    
                    if (pTDPool->hThreads[i] == NULL)
                    {
                        ExitProcess(GetLastError());
                    }
                }
                
                pTDPool->nThreads = nThreadsMaxSize;
            }
            else 
            {
                _tprintf(_TEXT("Ошибка выделения памяти для потоков"));
            }

        }
    }
    else 
    {
        _tprintf(_TEXT("Ошибка в выделении памяти для пула потоков"));
    }
    
    return (HTHDPOOL) pTDPool;
}

BOOL DestroyThreadPool(HTHDPOOL hThPool)
{
    PTHDPOOL pTDPool = (PTHDPOOL) hThPool;
    BOOL state = TRUE;
    
    for (int i = 0; i < pTDPool->nThreads; i++)
    {
        TerminateThread(pTDPool->hThreads[i], 0);
    }

    state = state && DestroyQueue(pTDPool->hBlockingQueue);
    state = state && HeapFree(GetProcessHeap(), 0, pTDPool->hThreads);
    state = state && HeapFree(GetProcessHeap(), 0, pTDPool);

    return state;
}

INT GetExecutedTasksCount(HTHDPOOL hTDPool)
{
    PTHDPOOL pTDPool = (PTHDPOOL) hTDPool;

    return pTDPool->nExecutedTasks; 
}

VOID SetExecutedTasksCount(HTHDPOOL hTDPool, int count)
{
    PTHDPOOL pTDPool = (PTHDPOOL) hTDPool;

    pTDPool->nExecutedTasks = count; 
}