#include "AllRace.h"

bool AllRace::hasRaceHappenedOnThisLine(string fn,int ln)
{
	map<ADDRINT,Race *>::iterator iter;
	for(iter=allRace.begin();iter!=allRace.end();iter++)  {
		if(iter->second)
			if(iter->second->fileName==fn && iter->second->lineNumber==ln)
				return true;
	}
	return false;
}

bool AllRace::isMemoryAlreadyHasRace(ADDRINT memoryAddr)
{
	if(allRace.find(memoryAddr)==allRace.end())
		return false;
	return true;
}

int AllRace::getRaceCount()
{
	return allRace.size();
}

void AllRace::addRace(Race *race)
{
	if(race) {
		if(isMemoryAlreadyHasRace(race->memoryAddr) || 
			hasRaceHappenedOnThisLine(race->fileName,race->lineNumber) ) {
			delete race;
			return ;
		}
		allRace[race->memoryAddr]=race;
	}
}
