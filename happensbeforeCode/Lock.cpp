#include "Lock.h"

Lock::Lock()
{
	this->addr=INITIAL_LOCK_ADDR;
	this->lockType=WRITER;
	this->lockVectorClock=new VectorClock(NON_THREAD_VECTOR_CLOCK);
}

Lock::Lock(ADDRINT addr)
{
	this->addr=addr;
	this->lockType=WRITER;
	this->lockVectorClock=new VectorClock(NON_THREAD_VECTOR_CLOCK);
}

bool Lock::operator<(const Lock &vRight) const
{
	return this->addr < vRight.addr;
}

bool Lock::operator>(const Lock &vRight) const
{
	return this->addr > vRight.addr;
}

bool Lock::operator==(const Lock &vRight) const
{
	return this->addr == vRight.addr;
}

