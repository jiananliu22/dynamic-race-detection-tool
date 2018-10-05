#ifndef __VECTORCLOCK_H
#define __VECTORCLOCK_H

#include "Header.h"
#include <iomanip>
#include <string.h>

#define NON_THREAD_VECTOR_CLOCK -1

/**
 * Happens-Before Vector_Clock Struct
 */
class VectorClock {
private:
	UINT32 *v;//所以v是一个指向数组的指针？
public:
	int processId;
	static int totalProcessCount;

	VectorClock(int processId);
	VectorClock(VectorClock *inClockPtr,int processId);
	~VectorClock();

	void receiveAction(VectorClock *vectorClockReceived);
	void receiveActionFromSpecialPoint(VectorClock *vectorClockReceived,UINT32 specialPoint);
	UINT32 *getValues() const;

	inline void event(){v[processId]++;};
	inline void sendEvent();
	
	bool happensBefore(VectorClock *input);
	bool isUniqueValue(int processId);

	VectorClock &operator++(); //prefix
	VectorClock operator++(int x);//postfix

	bool operator==(const VectorClock &vRight);//重载运算符
	bool operator!=(const VectorClock &vRight);
	bool operator<(const VectorClock &vRight);
	bool operator<=(const VectorClock &vRight);
	bool areConcurrent(VectorClock *vectorClockReceived);

	friend ostream& operator<<(ostream &os,const VectorClock &v);
};

#endif /* __VECTORCLOCK_H */