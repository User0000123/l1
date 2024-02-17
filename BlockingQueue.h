#ifndef __CONCURRENT_QUEUE_H__
#define __CONCURRENT_QUEUE_H__

#include <windows.h>

typedef LONG_PTR HBLOCKQUEUE;

HBLOCKQUEUE CreateQueue(int nQueueMaxElements);
BOOL DestroyQueue(HBLOCKQUEUE hConQueue);
BOOL Append(HBLOCKQUEUE hQueue, LPVOID data);
BOOL Remove(HBLOCKQUEUE hQueue, LPVOID* buffer);

#endif

