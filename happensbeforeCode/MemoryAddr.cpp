#include "MemoryAddr.h"

MemoryAddr::MemoryAddr(ADDRINT addr)
{
	this->address=addr;
	this->writeVectorClock=new VectorClock(NON_THREAD_VECTOR_CLOCK); //把vc改成epoch form
	this->readVectorClock=new VectorClock(NON_THREAD_VECTOR_CLOCK);
	this->mallocedAddrStart=addr;
}

