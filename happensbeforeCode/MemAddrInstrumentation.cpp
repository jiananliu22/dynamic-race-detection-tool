#include "MemAddrInstrumentation.h"
#include "AllMemoryAddress.h"
#include "ThreadLocalData.h"
#include "AllRace.h"
#include "Lock.h"

ADDRINT STACK_PTR_ERROR=1000000;
AllRace allRace;
int raceCounter=0;


extern PIN_LOCK variableLock;  
extern map<UINT64,MemoryAddr *> variableHashMap;  //去别的模块找它的定义
extern map<ADDRINT,ADDRINT> mallocPool;//前一个addrint是begin address 后一个是size

/**
 * Report race for each malloced area
 */
MallocArea findMallocStartAreaIfMallocedArea(ADDRINT addr)
{
	MallocArea retVal(false,addr);
	//first address higher than addr
	map<ADDRINT,ADDRINT>::iterator iter=mallocPool.upper_bound(addr);//对于已排序的序列：addr插入最晚的节点 //比如这个addr是3 这个map 的key 是1 2 2 4 所以iter(index)为3: 第二个2后面（因为3 > 2小于4）
	//this address is not in any malloced memory range
	if(iter==mallocPool.begin())
		return retVal;
	//nearest malloced memory start address
	--iter;
	uint32_t diff=abs(static_cast<int>(addr-iter->first));

	//if in a malloced memory range
	if(diff<=(uint32_t)iter->second) {
		retVal.mallocedAddrStart=iter->first;
		retVal.isMallocArea=true;
		retVal.size=(uint32_t)iter->second;
	}
	return retVal;
}

/**
 * Decide if the effective address is a global address
 */

bool isMemoryGlobal(ADDRINT addr,ADDRINT stackPtr)
{
	//if stack pointer is greater , address is in global or heap region
	if(abs(stackPtr-addr) > STACK_PTR_ERROR )
		return true;
	return false;
}

/**
 * Decide if we should consider this memory
 */
bool shouldMemoryBeConsidered(ADDRINT addr,ADDRINT stackPtr)
{
	if(isMemoryGlobal(addr,stackPtr) && !allRace.isMemoryAlreadyHasRace(addr))
		return true;
	return false;
}

/**
 * 
 */
Race * isRace(MemoryAddr *currentMemory,ADDRINT insPtr,UINT32 addressSize,bool isWrite ,
	VectorClock *threadClock , ADDRINT stackPtr)
{
	//at least a write operation have been accessed the memory
	if(currentMemory->writeVectorClock->areConcurrent(threadClock)
		|| (isWrite && currentMemory->readVectorClock->areConcurrent(threadClock)) )
		return new Race("",0,currentMemory->address,insPtr,addressSize,stackPtr);
	return NULL;
}

/**
 *  Instrument after the read instruction
 */
VOID MemoryReadInstrumentation(THREADID threadId,ADDRINT addr,ADDRINT stackPtr,const char *imageName,
	ADDRINT insPtr,UINT32 readSize)
{
	
	if(!shouldMemoryBeConsidered(addr,stackPtr))
		return ;
	ThreadLocalData *tls=getTLS(threadId);
	VectorClock *threadClock=tls->currentVectorClock;

	PIN_GetLock(&variableLock,threadId+1);
	MemoryAddr *memory=NULL;
	//address not exists
	if(variableHashMap.find(addr)==variableHashMap.end())
		variableHashMap[addr]=new MemoryAddr(addr);
	memory=variableHashMap[addr];

	//cout<<"============Read before receive=============\n";
	//cout<<"variable write clock:"<<*memory->writeVectorClock;
	//cout<<"current thread clock"<<*threadClock;

	memory->readVectorClock->receiveActionFromSpecialPoint(threadClock,threadId);
	//must not be only within a thread
	if(!memory->writeVectorClock->isUniqueValue(threadId)) {//need change by using epoch form cos wirte - read

		Race *race=isRace(memory,insPtr,readSize,false,threadClock,stackPtr);//need change
		if(race) {
			MallocArea mArea=findMallocStartAreaIfMallocedArea(addr);
			memory->mallocedAddrStart=mArea.mallocedAddrStart;
			allRace.addRace(race);
			race->setRaceInfo();
		}
	}
	PIN_ReleaseLock(&variableLock);
}


VOID MemoryWriteInstrumentation(THREADID threadId,ADDRINT addr,ADDRINT stackPtr,const char *imageName,
	ADDRINT insPtr,UINT32 writeSize)
{
	if(!shouldMemoryBeConsidered(addr,stackPtr))
		return ;

	ThreadLocalData *tls=getTLS(threadId);
	VectorClock *threadClock=tls->currentVectorClock;

	MemoryAddr *memory=NULL;

	PIN_GetLock(&variableLock,threadId);
	if(variableHashMap.find(addr)==variableHashMap.end())
		variableHashMap[addr]=new MemoryAddr(addr);
	memory=variableHashMap[addr];

	memory->writeVectorClock->receiveActionFromSpecialPoint(threadClock,threadId);//change by using wirte epoch

	bool shouldCheckRace=true;

	//cout<<"============Write before receive=============\n";
	//cout<<"variable write clock:"<<*memory->writeVectorClock;
	//cout<<"current thread clock"<<*threadClock;

	//read and write are both within the same thread
	if(memory->writeVectorClock->isUniqueValue(threadId) && memory->readVectorClock->isUniqueValue(threadId))//change by using wirte epoch	
		shouldCheckRace=false;//理由这是检查内存 而且检出race的就不会再检查了 所以如果一般情况下VC都是0，除了当前的thread：解释isUniqueValue

	if(shouldCheckRace) {
		Race *race=isRace(memory,insPtr,writeSize,true,threadClock,stackPtr);// change 分别 用这个 和 别的
		if(race) {
			MallocArea mArea=findMallocStartAreaIfMallocedArea(addr);
			memory->mallocedAddrStart=mArea.mallocedAddrStart;

			allRace.addRace(race);
			race->setRaceInfo();
		}
	}

	cout<<"======ThreadId: "<<threadId<<" Write after receive=======\n";
	cout<<"variable write clock:"<<*memory->writeVectorClock;
	cout<<"variable read clock:"<<*memory->readVectorClock;
	cout<<"current thread clock:"<<*threadClock;

	PIN_ReleaseLock(&variableLock);
}

/**
 * for every write instruction and instrumentation
 */
void processMemoryWriteInstruction(INS ins,const char *imageName) 
{
	UINT32 memoryOperandCount=INS_MemoryOperandCount(ins);//内存操作数数目
	
	for(UINT32 i=0;i<memoryOperandCount;i++) {
		//write instruction
		if(INS_MemoryOperandIsWritten(ins,i)) {//TRUE if memory operand memopIdx：i is written 
			INS_InsertPredicatedCall(
				ins,IPOINT_BEFORE,(AFUNPTR)MemoryWriteInstrumentation,
				IARG_THREAD_ID,
				//Effective address of a memory op (memory op index is next arg); only valid at IPOINT_BEFORE.
				IARG_MEMORYOP_EA,i,
				//Type: ADDRINT for integer register. Value of a register (additional register arg required)
				IARG_REG_VALUE,REG_STACK_PTR,
				//Type: "VOID *". Constant value (additional pointer arg required). 
				IARG_PTR,imageName,
				//Type: ADDRINT. The address of the instrumented instruction
				IARG_INST_PTR,
				//
				IARG_MEMORYWRITE_SIZE,
				IARG_CALL_ORDER,CALL_ORDER_FIRST+30,
				IARG_END);
		}
	}
	//cout<<"write instruction finish"<<endl;
}

void processMemoryReadInstruction(INS ins,const char* imagename)
{
	UINT32 memoryOperandCount=INS_MemoryOperandCount(ins);
	for(UINT32 i=0;i<memoryOperandCount;i++) {
		if(INS_MemoryOperandIsRead(ins,i)) {
			INS_InsertPredicatedCall(
				ins,IPOINT_BEFORE,(AFUNPTR)MemoryReadInstrumentation,
				IARG_THREAD_ID,
				IARG_MEMORYOP_EA,i,
				IARG_REG_VALUE,REG_STACK_PTR,//pass current stack ptr
				IARG_PTR,imagename,
				IARG_INST_PTR,
				IARG_MEMORYWRITE_SIZE,
				IARG_CALL_ORDER,CALL_ORDER_FIRST+30,
				IARG_END);
		}
	}
	//cout<<"read instruction finish"<<endl;
}

/**
 * Trace=>BBL=>Instruction=>Variable
 */
VOID InstrumentTrace(TRACE trace,VOID *v)
{
	//Find image by address. For each image,
	//check if the address is within the mapped memory region of one of its segments.
	IMG img=IMG_FindByAddress(TRACE_Address(trace));

	if(!IMG_Valid(img) || !IMG_IsMainExecutable(img))
		return ;
	const char* imageName = IMG_Name(img).c_str();

	
	//traverse the basic block in the trace
	//BBLs are only use to reach instructions
	for(BBL bbl=TRACE_BblHead(trace);BBL_Valid(bbl);bbl=BBL_Next(bbl)) {
		for(INS ins=BBL_InsHead(bbl);INS_Valid(ins);ins=INS_Next(ins)) {
			processMemoryWriteInstruction(ins,imageName);
			processMemoryReadInstruction(ins,imageName);
		}
	}
}



