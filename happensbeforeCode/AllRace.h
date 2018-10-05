#ifndef __ALLRACE_H
#define __ALLRACE_H

#include "Race.h"
#include <map>
using namespace std;

struct MallocArea {
	bool isMallocArea;
	ADDRINT mallocedAddrStart;
	ADDRINT size;

	MallocArea(bool b,ADDRINT a):isMallocArea(b),mallocedAddrStart(a)
	{
		size=0;
	}
};


class AllRace {
public:
	bool isMemoryAlreadyHasRace(ADDRINT memoryAddr);
	bool hasRaceHappenedOnThisLine(string fn,int ln);
	int getRaceCount();
	void addRace(Race *race);

	map<ADDRINT,Race *> allRace;//说明指针所指向的类型是Race
};

#endif