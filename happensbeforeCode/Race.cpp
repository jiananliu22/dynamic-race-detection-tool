#include "Race.h"
#include <sstream>
#include <math.h>

Race::Race(string fn,UINT32 ln,ADDRINT memAddr,ADDRINT insPtrIn,UINT32 addrSize,ADDRINT stackPtrIn)
{
	fileName=fn;
	lineNumber=ln;
	memoryAddr=memAddr;
	insPtr=insPtrIn;
	addressSize=addrSize;
	stackPtr=stackPtrIn;
}

void Race::setRaceInfo()
{
	string fileName;
	int lineNumber;

	PIN_LockClient();//pin的api需要用到这个
	PIN_GetSourceLocation(insPtr,NULL,&lineNumber,&fileName);//Find the line number, file, and column number corresponding to a memory address
	PIN_UnlockClient();//pin的api需要用到这个

	this->fileName=fileName;
	this->lineNumber=lineNumber;
}

string Race::toString()
{
	ostringstream os;
    os << endl << "-----------------------RACE INFO STARTS------------------:\n" << hex << memoryAddr  << dec << endl;
    os << "The Exact Place:"  << fileName << "@" << lineNumber << "   stackPtr:" << hex << stackPtr << dec   << endl;
    os << "Difference:" << abs((INT64)(stackPtr - memoryAddr)) << endl;

	os << "------------------RACE INFO ENDS-------------------\n";
	return os.str();
}
