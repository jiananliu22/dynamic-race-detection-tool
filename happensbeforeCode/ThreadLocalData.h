#ifndef __THREADLOCALDATA_H
#define __THREADLOCALDATA_H

#include "VectorClock.h"

class ThreadLocalData {
public:
	ThreadLocalData(THREADID tid);

	VectorClock *currentVectorClock;
	ADDRINT nextLockAddr;

	PIN_LOCK threadLock;
	bool isAlive;
	ADDRINT nextMallocSize;
};

ThreadLocalData *getTLS(THREADID tid);

#endif