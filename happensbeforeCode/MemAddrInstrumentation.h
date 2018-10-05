#ifndef __MEMADDRINSTRUMENTATION_H
#define __MEMADDRINSTRUMENTATION_H

#include "Header.h"

bool isMemoryGlobal(ADDRINT addr,ADDRINT stackPtr);

//Pin calls following two funcitons when a read or write encountered
VOID MemoryReadInstrumentation(THREADID threadId,ADDRINT addr,ADDRINT stackPtr,const char *imageName,
	ADDRINT insPtr,UINT32 readSize);

VOID MemoryWriteInstrumentation(THREADID threadId,ADDRINT addr,ADDRINT stackPtr,const char *imageName,
	ADDRINT insPtr,UINT32 writeSize);

//analysis routine
void processMemoryReadInstrumentation(INS ins,const char* imageName);
void processMemoryWriteInstrumentation(INS ins,const char *imageName);

VOID InstrumentTrace(TRACE trace,VOID *v);


#endif /* __MEMADDRINSTRUMENTATION_H */
