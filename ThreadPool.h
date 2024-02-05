#include <windows.h>

typedef LONG_PTR HTHDPOOL;
typedef void (*LPTASKFUNC)(LPVOID);

HTHDPOOL CreateThreadPool(int nTasksMaxSize, int nThreadsMaxSize);
BOOL DestroyThreadPool(HTHDPOOL hThPool);
BOOL SubmitTask(HTHDPOOL hThPool, LPTASKFUNC runnableTask, LPVOID lParams);
INT GetExecutedTasksCount(HTHDPOOL);
VOID SetExecutedTasksCount(HTHDPOOL hTDPool, int count);