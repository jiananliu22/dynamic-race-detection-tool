#ifndef __RACE_H
#define __RACE_H

#include "Header.h"

class Race {
public:
	Race(string fileName,UINT32 lineNumber,ADDRINT addr,ADDRINT insPtr,UINT32 addressSize, ADDRINT stackPtr);
	string toString();
	void setRaceInfo();

	int lineNumber;
	string fileName;
	ADDRINT memoryAddr;
	ADDRINT insPtr;
	ADDRINT stackPtr;
	UINT32 addressSize;
};

#endif