#ifndef __PTHREADCALLINSTRUMENTATION_h
#define __PTHREADCALLINSTRUMENTATION_h

#include "Header.h"
#include "VectorClock.h"
#define libpthread "libpthread"

VOID InstrumentImage(IMG img,VOID *v);

VOID GetLockAnalysis(ADDRINT arg1,THREADID tid,char *imageName,ADDRINT stackPtr);
VOID GetLockAnalysisEnter(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr);

VOID releaseLock(ADDRINT lockAddr,THREADID tid);
VOID createNewLock(ADDRINT lockAddr,THREADID tid);

VOID mutexTryLockExitAnalysis(ADDRINT exitVal,THREADID tid,char *imageName,ADDRINT stackPtr);
VOID mutexTryLockEnterAnalysis(ADDRINT lockAddr,THREADID id,char *imageName,ADDRINT stackPtr);

VOID PthreadJoinAnalysis(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr);
VOID PthreadCreateAnalysis(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr);

VOID LockInit(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr);

VOID ReleaseLockAnalysis(ADDRINT arg1,THREADID tid,char *imageName,ADDRINT stackPtr);

VOID MallocAfter(ADDRINT memoryAddrStart,THREADID tid);
VOID FreeEnter(CHAR *name,ADDRINT memoryAddrStart,THREADID tid);

VOID freeMemoryAddress(ADDRINT memAddrFreeStart,ADDRINT maxMemAddrToBeFreed,THREADID tid);

void initFunctionNameInstrumentPthread();

typedef struct {

	string functionName;
	AFUNPTR instrumentFunction;
	IPOINT iPoint;
	UINT16 callOrder;

} InstrumentationFunctionName;


#endif /* __PTHREADCALLINSTRUMENTATION_h */