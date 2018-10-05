#ifndef __MEMORYADDR_H
#define __MEMORYADDR_H

#include "pin.H"
#include "VectorClock.h"

class MemoryAddr{
public:
	MemoryAddr(ADDRINT addr);
	ADDRINT address;
	VectorClock *writeVectorClock;
	VectorClock *readVectorClock;
	ADDRINT mallocedAddrStart;
};

#endif