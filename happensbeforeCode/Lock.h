#ifndef __LOCK_H
#define __LOCK_H

#include "pin.H"
#include "VectorClock.h"

#define INITIAL_LOCK_ADDR -99999

enum LockType {
	READER,
	WRITER
};


class Lock {
public:
	Lock(ADDRINT addr);
	Lock();
	VectorClock *lockVectorClock;
	LockType lockType;
	ADDRINT addr;

	bool operator<(const Lock &vRight) const;
	bool operator>(const Lock &vRight) const;
	bool operator==(const Lock &vRight) const;
};

#endif