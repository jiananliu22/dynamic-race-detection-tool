#include "PthreadCallInstrumentation.h"
#include "ThreadLocalData.h"
#include "AllMemoryAddress.h"
#include "Lock.h"

using namespace std;

#define MALLOC "malloc"
#define FREE "free"
#define CALLOC "calloc"
#define REALLOC "realloc"

vector<InstrumentationFunctionName> functionNamesEntry;
vector<InstrumentationFunctionName> functionNamesExit;

int lockCounter=0;

map<ADDRINT,ADDRINT> mallocPool;
map<ADDRINT,Lock *> allLock;
PIN_LOCK mallocPoolLock;
PIN_LOCK lockAcqRel;

extern map<UINT64,MemoryAddr *> variableHashMap;
extern PIN_LOCK variableLock;

int mallocPoolMaxSize=0;


/**
 * For all instrumentaition to pthread lock calls are similay,so the way
 * we do instrumentation is the same.We need some initilization for function_name
 * and instrumentation_function_name mapping .
 */
void initFunctionNameInstrumentPthread()
{
	PIN_InitLock(&mallocPoolLock);
	PIN_InitLock(&lockAcqRel);

	//pthread_mutex_init
	InstrumentationFunctionName mutexInit;
	mutexInit.functionName="pthread_mutex_init";
	mutexInit.instrumentFunction=(AFUNPTR)LockInit;
	mutexInit.iPoint=IPOINT_BEFORE; //ipoint： Determines where the analysis call is inserted relative to the instrumented object //Insert a call before an instruction or routine. 
	mutexInit.callOrder=2;

	functionNamesExit.push_back(mutexInit);//新增元素至vector 的尾端，必要時會進行記憶體配置。

	//pthread_mutex_lock exit
	InstrumentationFunctionName mutexGetExit;
	mutexGetExit.functionName="pthread_mutex_lock";
	mutexGetExit.instrumentFunction=(AFUNPTR)GetLockAnalysis;
	mutexGetExit.iPoint=IPOINT_AFTER;//Insert a call on the fall through path of an instruction or return path of a routine.
	mutexGetExit.callOrder=2;

	functionNamesExit.push_back(mutexGetExit);

	//pthread_mutex_trylock exit
	InstrumentationFunctionName mutexTryLockExit;
	mutexTryLockExit.functionName="pthread_mutex_trylock";
	mutexTryLockExit.instrumentFunction=(AFUNPTR)mutexTryLockExitAnalysis;
	mutexTryLockExit.iPoint=IPOINT_AFTER;
	mutexTryLockExit.callOrder=2;

	functionNamesExit.push_back(mutexTryLockExit);	

	//pthread_mutex_trylock enter
	InstrumentationFunctionName mutexTryLockEnter;
	mutexTryLockEnter.functionName="pthread_mutex_trylock";
	mutexTryLockEnter.instrumentFunction=(AFUNPTR)mutexTryLockEnterAnalysis;
	mutexTryLockEnter.iPoint=IPOINT_BEFORE;
	mutexTryLockEnter.callOrder=1;

	functionNamesEntry.push_back(mutexTryLockEnter);	

	//pthread_mutex_join enter
	InstrumentationFunctionName pthreadJoin;
	pthreadJoin.functionName="pthread_join";
	pthreadJoin.instrumentFunction=(AFUNPTR)PthreadJoinAnalysis;
	pthreadJoin.iPoint=IPOINT_BEFORE;
	pthreadJoin.callOrder=1;

	functionNamesEntry.push_back(pthreadJoin);	

	//pthread_mutex_create enter
	InstrumentationFunctionName pthreadCreate;
	pthreadCreate.functionName="pthread_create";
	pthreadCreate.instrumentFunction=(AFUNPTR)PthreadCreateAnalysis;
	pthreadCreate.iPoint=IPOINT_BEFORE;
	pthreadCreate.callOrder=1;

	functionNamesEntry.push_back(pthreadCreate);		

	//pthread_mutex_lock enter
	InstrumentationFunctionName mutexGet;
	mutexGet.functionName="pthread_mutex_lock";
	mutexGet.instrumentFunction=(AFUNPTR)GetLockAnalysisEnter;
	mutexGet.iPoint=IPOINT_BEFORE;
	mutexGet.callOrder=1;

	functionNamesEntry.push_back(mutexGet);	




	//pthread_rwlock_wrlock enter
	InstrumentationFunctionName rwWriteGet;
	rwWriteGet.functionName="pthread_rwlock_wrlock";
	rwWriteGet.instrumentFunction=(AFUNPTR)GetLockAnalysisEnter;
	rwWriteGet.iPoint=IPOINT_BEFORE;
	rwWriteGet.callOrder=1;

	functionNamesEntry.push_back(rwWriteGet);

	//pthread_mutex_unlock
	InstrumentationFunctionName mutexRelease;
	mutexRelease.functionName="pthread_mutex_unlock";
	mutexRelease.instrumentFunction=(AFUNPTR)ReleaseLockAnalysis;
	mutexRelease.iPoint=IPOINT_BEFORE;
	mutexRelease.callOrder=5;

	functionNamesEntry.push_back(mutexRelease);	

	//pthread_rwlock_wrlock
	InstrumentationFunctionName rwLockWriteLock;
	rwLockWriteLock.functionName="pthread_rwlock_wrlock";
	rwLockWriteLock.instrumentFunction=(AFUNPTR)GetLockAnalysis;
	rwLockWriteLock.iPoint=IPOINT_AFTER;
	rwLockWriteLock.callOrder=2;

	functionNamesEntry.push_back(rwLockWriteLock);	


	//pthread_rwlock_unlock enter
	InstrumentationFunctionName rwLockUnlock;
	rwLockUnlock.functionName="pthread_rwlock_unlock";
	rwLockUnlock.instrumentFunction=(AFUNPTR)ReleaseLockAnalysis;
	rwLockUnlock.iPoint=IPOINT_BEFORE;
	rwLockUnlock.callOrder=5;

	functionNamesEntry.push_back(rwLockUnlock);	

	//pthread_rwlock_rdlock
	InstrumentationFunctionName rwLockReadLock;
	rwLockReadLock.functionName="pthread_rwlock_rdlock";
	rwLockReadLock.instrumentFunction=(AFUNPTR)GetLockAnalysis;
	rwLockReadLock.iPoint=IPOINT_AFTER;
	rwLockReadLock.callOrder=4;

	functionNamesEntry.push_back(rwLockReadLock);	

	//pthread_rwlock_rdlock enter
	InstrumentationFunctionName rwLockReadLockEnter;
	rwLockReadLockEnter.functionName="pthread_rwlock_rdlock";
	rwLockReadLockEnter.instrumentFunction=(AFUNPTR)GetLockAnalysisEnter;
	rwLockReadLockEnter.iPoint=IPOINT_BEFORE;
	rwLockReadLockEnter.callOrder=3;

	functionNamesEntry.push_back(rwLockReadLockEnter);			
}

/**
 * before mallocing 在内存分配前
 */
VOID MallocEnter(CHAR *name,ADDRINT size,THREADID tid)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	//enter the malloc routine ,having not been formalized malloced 
	tls->nextMallocSize=size;
	cout<<"Malloc size:"<<size<<endl;
	PIN_ReleaseLock(&tls->threadLock);
}

/**
 * after mallocing
 */

VOID MallocAfter(ADDRINT memAddrStart,THREADID tid)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	ADDRINT mallocSize=tls->nextMallocSize;
	//clear for next malloc routine execute
	tls->nextMallocSize=0;
	PIN_ReleaseLock(&tls->threadLock);
	//add to malloc pool
	PIN_GetLock(&mallocPoolLock,tid+1);
	mallocPool[memAddrStart]=mallocSize;//mallocPool is map
	cout<<"malloc return : start address:"<<hex<<memAddrStart<<dec<<" mallocSize:"<<mallocSize<<endl;

	int tmpSize=mallocPool.size();
	if(tmpSize>mallocPoolMaxSize)
		mallocPoolMaxSize=tmpSize;

	PIN_ReleaseLock(&mallocPoolLock);
}

VOID FreeEnter(CHAR *name,ADDRINT memAddrFreeStart,THREADID tid)
{
	PIN_GetLock(&mallocPoolLock,tid+1);
	cout<<"Free Called:"<<hex<<memAddrFreeStart<<" tid:"<<tid<<endl;

	ADDRINT freeSize=mallocPool[memAddrFreeStart];

	mallocPool[memAddrFreeStart]=0;
	mallocPool.erase(memAddrFreeStart);

	PIN_ReleaseLock(&mallocPoolLock);

	if(freeSize==0)
		return ;
	ADDRINT maxMemAddrToBeFreed=memAddrFreeStart+freeSize;
	cout<<"Free address start:"<<hex<<memAddrFreeStart<<" size:"<<dec<<freeSize<<endl;

	freeMemoryAddress(memAddrFreeStart,maxMemAddrToBeFreed,tid);
}

/**
 * free all variables' address range in malloced memory
 */
VOID freeMemoryAddress(ADDRINT memAddrFreeStart,ADDRINT maxMemAddrToBeFreed,THREADID tid)
{
	for(ADDRINT currAddr=memAddrFreeStart;currAddr<=maxMemAddrToBeFreed;++currAddr) {
		PIN_GetLock(&variableLock,tid+1);
		//find variables used these memory
		if(variableHashMap.find(currAddr)==variableHashMap.end()) {
			PIN_ReleaseLock(&variableLock);
			continue;
		}

		delete variableHashMap[currAddr]->writeVectorClock; //改 把变量写VC改成epoch form
		delete variableHashMap[currAddr]->readVectorClock;

		variableHashMap.erase(currAddr);
		PIN_ReleaseLock(&variableLock);
	}
}

void instrumentMallocFree(IMG img)
{
	//find the malloc function
	//instrument before entry and after exit
	RTN mallocRtn=RTN_FindByName(img,MALLOC);
	if(RTN_Valid(mallocRtn)) {
		RTN_Open(mallocRtn);

		RTN_InsertCall(mallocRtn,IPOINT_BEFORE,(AFUNPTR)MallocEnter,
			//ADDRINT. Constant value (additional arg required).
			IARG_ADDRINT,MALLOC,
			//Type: ADDRINT. Integer argument n. Valid only at the entry
			//point of a routine. (First argument number is 0.) 
			IARG_FUNCARG_ENTRYPOINT_VALUE,0,
			IARG_THREAD_ID,
			IARG_END);

		RTN_InsertCall(mallocRtn,IPOINT_AFTER,(AFUNPTR)MallocAfter,
			//Type: ADDRINT. Function result. Valid only at return instruction. 
			IARG_FUNCRET_EXITPOINT_VALUE,
			IARG_THREAD_ID,
			IARG_END);

		RTN_Close(mallocRtn);
	}

	//find the free function
	RTN freeRtn=RTN_FindByName(img,FREE);
	if(RTN_Valid(freeRtn)) {
		RTN_Open(freeRtn);

		RTN_InsertCall(freeRtn,IPOINT_BEFORE,(AFUNPTR)FreeEnter,
			IARG_ADDRINT,FREE,
			IARG_FUNCARG_ENTRYPOINT_VALUE,0,
			IARG_THREAD_ID,
			IARG_END);

		RTN_Close(freeRtn);
	}
}

/**
 * pthread lib and malloc-free lib
 */
VOID InstrumentImage(IMG img,VOID *v)
{
	char *imageName=const_cast<char *>(IMG_Name(img).c_str());
	instrumentMallocFree(img);

	//we only instrument pthread function calls
	if(string(imageName).find(libpthread)==string::npos)
		return ;

	//traverse lock functions that needed instrumentation
	//for all pthread lock family functions have the similar declaration
	for(UINT32 i=0;i<functionNamesEntry.size();i++) {
		InstrumentationFunctionName currentType=functionNamesEntry.at(i);

		RTN pthreadMutexRTN=RTN_FindByName(img,currentType.functionName.c_str());

		if(RTN_Valid(pthreadMutexRTN)) {
			RTN_Open(pthreadMutexRTN);
			//instrument pthread_mutex_lock to print the input argument value
			//and the return value
			RTN_InsertCall(pthreadMutexRTN,currentType.iPoint,
				(AFUNPTR)currentType.instrumentFunction,
				IARG_FUNCARG_ENTRYPOINT_VALUE,0,
				IARG_THREAD_ID,
				//string variable pointer
				IARG_PTR,imageName,
				IARG_REG_VALUE,REG_STACK_PTR,
				IARG_CALL_ORDER,CALL_ORDER_FIRST+currentType.callOrder,
				IARG_END);

			RTN_Close(pthreadMutexRTN);
		}
	}

	for(UINT32 i=0;i<functionNamesExit.size();i++) {
		InstrumentationFunctionName currentType=functionNamesExit.at(i);

		RTN pthreadMutexRTN=RTN_FindByName(img,currentType.functionName.c_str());
		if(RTN_Valid(pthreadMutexRTN)) {
			RTN_Open(pthreadMutexRTN);

			RTN_InsertCall(pthreadMutexRTN,currentType.iPoint,
				(AFUNPTR)currentType.instrumentFunction,
				IARG_FUNCRET_EXITPOINT_VALUE,
				IARG_THREAD_ID,
				IARG_PTR,imageName,
				IARG_REG_VALUE,REG_STACK_PTR,
				IARG_CALL_ORDER,CALL_ORDER_FIRST+currentType.callOrder,
				IARG_END);

			RTN_Close(pthreadMutexRTN);
		}
	}
}

//==================followings are instrumented analysis functions=======================

VOID PthreadCreateAnalysis(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	//PIN_GetTid()-Get system identifier of the current thread. 
	cout << "****************  THREAD CREATED  ***************  " << tid << "   pin id:"  
	<< PIN_ThreadId()  << "  pin id2:" << PIN_GetTid()<< endl;
	PIN_ReleaseLock(&tls->threadLock);
}

VOID PthreadJoinAnalysis(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	cout << "****************  THREAD JOINED  ***************  " << tid << "   pin id:"  
	<< PIN_ThreadId()  << "  pin id2:" << PIN_GetTid()<< endl;
	PIN_ReleaseLock(&tls->threadLock);	
}

//======================================================================================
/**
 * We take the lock as a intermedian between the lock() and unlock() synchronizaiton 
 * operation , so our vector clock will be attached to it and transferrd between sender
 * and receiver .
 */
//======================================================================================

VOID createNewLock(ADDRINT lockAddr,THREADID tid)
{
	PIN_GetLock(&lockAcqRel,tid);
	if(allLock.find(lockAddr)==allLock.end()) {
		Lock *tmp=new Lock(lockAddr);
		allLock[lockAddr]=tmp;
	}
	PIN_ReleaseLock(&lockAcqRel);
}

/**
 * Thread release the lock (triggle a event) , update lock's vector clock to be sent
 * generally be used after initialize the lock or get the lock
 */
VOID releaseLock(ADDRINT lockAddr,THREADID tid)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	//current thread triggle a event
	tls->currentVectorClock->event();
	PIN_ReleaseLock(&tls->threadLock);
	//lock's vector clock should be updated the lastest info of current thread's vector clock 
	PIN_GetLock(&lockAcqRel,tid);
	if(allLock.find(lockAddr)!=allLock.end()) {
		Lock *tmp=allLock[lockAddr];
		tmp->lockVectorClock->receiveAction(tls->currentVectorClock);
	}
	PIN_ReleaseLock(&lockAcqRel);
}

/**
 * Enter the get-lock routine,create the lock
 */
VOID GetLockAnalysisEnter(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	tls->nextLockAddr=lockAddr;
	PIN_ReleaseLock(&tls->threadLock);
	//create the lock or not
	createNewLock(lockAddr,tid);
}


VOID mutexTryLockEnterAnalysis(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	ThreadLocalData *tls=getTLS(tid);
	PIN_GetLock(&tls->threadLock,tid+1);
	tls->nextLockAddr=lockAddr;
	PIN_ReleaseLock(&tls->threadLock);
	createNewLock(lockAddr,tid);
}


VOID LockInit(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	createNewLock(lockAddr,tid);
	//after new a lock , should send notification , other threads can acquire the lock
	releaseLock(lockAddr,tid);
}

/**
 * After get the lock , current thread's vector clock should be updated the 
 * lastest info of lock's vector clock
 */
VOID GetLockAnalysis(ADDRINT lockAddr, THREADID tid ,char* imageName, ADDRINT stackPtr)
{
	ThreadLocalData* tls = getTLS(tid);
	PIN_GetLock(&tls->threadLock, tid+1);
	lockAddr=tls->nextLockAddr;
	PIN_ReleaseLock(&tls->threadLock);
	//get the lock info 
	Lock *tmp=NULL;
	PIN_GetLock(&lockAcqRel,tid);
	if(allLock.find(lockAddr)!=allLock.end())
		tmp=allLock[lockAddr];
	else {
		cout<<"gotton lock is not alive"<<hex<<lockAddr<<dec<<endl;
		PIN_ReleaseLock(&lockAcqRel);
		return ;
	}
	PIN_ReleaseLock(&lockAcqRel);
	//current thread's vector clock should update with the lock vector clock
	PIN_GetLock(&tls->threadLock,tid);
	//acquire the lock
	tls->currentVectorClock->receiveAction(tmp->lockVectorClock);
	PIN_ReleaseLock(&tls->threadLock);
}

/**
 * After try lock return
 */
VOID mutexTryLockExitAnalysis(ADDRINT exitVal,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	//pthread_mutex_trylock return 0 indicates that success
	if(exitVal==0) {
		ThreadLocalData *tls=getTLS(tid);
		PIN_GetLock(&tls->threadLock,tid+1);
		ADDRINT lockAddr=tls->nextLockAddr;
		PIN_ReleaseLock(&tls->threadLock);
		
		releaseLock(lockAddr,tid);
	}
}

/**
 * 
 */
VOID ReleaseLockAnalysis(ADDRINT lockAddr,THREADID tid,char *imageName,ADDRINT stackPtr)
{
	releaseLock(lockAddr,tid);
}



