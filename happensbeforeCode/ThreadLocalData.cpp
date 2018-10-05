#include "ThreadLocalData.h"
extern TLS_KEY tls_key;

ThreadLocalData::ThreadLocalData(THREADID tid)
{
	currentVectorClock=new VectorClock(tid);
	nextLockAddr=0;
	PIN_InitLock(&threadLock);//Initialize the lock as free
	isAlive=true;
	nextMallocSize=0;
}

ThreadLocalData *getTLS(THREADID tid)
{
	ThreadLocalData *tls=static_cast<ThreadLocalData *>(PIN_GetThreadData(tls_key,tid)); //Get the value stored in the specified TLS slot of the thread
	return tls;
}

