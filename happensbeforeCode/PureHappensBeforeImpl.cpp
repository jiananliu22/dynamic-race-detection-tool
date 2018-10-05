#include "Header.h"
#include "PthreadCallInstrumentation.h"
#include "ThreadLocalData.h"
#include "MemoryAddr.h"
#include "MemAddrInstrumentation.h"
#include "AllMemoryAddress.h"
#include "AllRace.h"

PIN_LOCK thread_lock;
TLS_KEY tls_key;
PIN_LOCK variableLock;


extern map<UINT64,MemoryAddr *> variableHashMap;
extern AllRace allRace;
map<THREADID,THREADID> mapOfThreadIds;

int totalThreadCount=0;

VOID ThreadStart(THREADID threadId,CONTEXT *ctxt,INT32 flags,VOID *v)  //THREADID： Thread ID assigned by PIN
{
	cout<<"Thread Start:"<<threadId<<endl;

	//main thread
	if(threadId==0) {
		PIN_GetLock(&thread_lock,threadId+1);
		mapOfThreadIds[threadId]=PIN_GetTid();//Get system identifier of the current thread
		PIN_ReleaseLock(&thread_lock);
		ThreadLocalData *tls=new ThreadLocalData(threadId);
		PIN_SetThreadData(tls_key,tls,threadId);//Store specified value in the specified TLS slot of the thread
		return ;
	}

	PIN_GetLock(&thread_lock,threadId+1);
	//store the identical system tid
	mapOfThreadIds[threadId]=PIN_GetTid();
	PIN_ReleaseLock(&thread_lock);

	//we assumed that parent thread id is 0
	ThreadLocalData *parentTls=getTLS(0);
	//set the thread's local storage
	ThreadLocalData *tls=new ThreadLocalData(threadId);
	PIN_SetThreadData(tls_key,tls,threadId);

	PIN_GetLock(&parentTls->threadLock,threadId+1);
	PIN_GetLock(&tls->threadLock,threadId+1);

	++totalThreadCount;
	//parent thread execute the create thread event
	parentTls->currentVectorClock->event();
	//send notification to child thread
	tls->currentVectorClock->receiveAction(parentTls->currentVectorClock);

	PIN_ReleaseLock(&tls->threadLock);//Release the lock.
	PIN_ReleaseLock(&parentTls->threadLock);
}

VOID ThreadFini(THREADID threadId,const CONTEXT *ctxt,INT32 code,VOID *v)
{
	if(threadId==0)
		return ;

	cout<<"Thread Finish:"<<threadId<<endl;
	PIN_GetLock(&thread_lock,threadId);
	mapOfThreadIds[threadId]=PIN_GetTid();
	PIN_ReleaseLock(&thread_lock);

	//we assume that parent thread id is 0
	THREADID parentThreadId=0;
	ThreadLocalData *parentTls=getTLS(parentThreadId);
	ThreadLocalData *tls=getTLS(threadId);
	tls->isAlive=false;
	PIN_GetLock(&parentTls->threadLock,parentThreadId);
	PIN_GetLock(&tls->threadLock,parentThreadId);
	//receive notification  from child thread
	tls->currentVectorClock->event();
	parentTls->currentVectorClock->receiveAction(tls->currentVectorClock);

	PIN_ReleaseLock(&tls->threadLock);
	PIN_ReleaseLock(&parentTls->threadLock);
}

extern map<ADDRINT,ADDRINT> mallocPoll;
extern int mallocPoolMaxSize;

VOID Fini(INT32 code ,VOID *v)
{
	cout<<"Total race count :"<<allRace.getRaceCount()<<endl;
	map<ADDRINT,Race*>::iterator iter;
	for(iter=allRace.allRace.begin();iter!=allRace.allRace.end();iter++) { 
		if(iter->second) {
			cout<<iter->second->toString()<<endl;
		}
	}
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    PIN_ERROR("This Pintool prints a trace of malloc calls in the guest application\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(INT32 argc, CHAR **argv)
{

	initFunctionNameInstrumentPthread();

	PIN_InitSymbols();//Initialize symbol table code. Pin does not read symbols unless this is called.
	
    // // Initialize pin
	if(PIN_Init(argc,argv))//Initialize Pin system
		return Usage();

	tls_key=PIN_CreateThreadDataKey(0);
	
	PIN_InitLock(&thread_lock);//Initialize the lock as free
	PIN_InitLock(&variableLock);//Initialize the lock as free

	PIN_AddThreadStartFunction(ThreadStart,0);//for thrad start instrumentation
	PIN_AddThreadFiniFunction(ThreadFini,0);//for thread finish instrumentation
	IMG_AddInstrumentFunction(InstrumentImage,0);//for function call instrumentation,PthreadCallInstrumentation
	
	TRACE_AddInstrumentFunction(InstrumentTrace,0);////for instruction指令 read/write instrumentation,MemAddrInstrumentation
	PIN_AddFiniFunction(Fini,0);//Call func immediately before the application exits
	PIN_StartProgram();
	return 0;
}
