#include "VectorClock.h"

int VectorClock::totalProcessCount=8;

ostream &operator<<(ostream &os,const VectorClock &v)
{
	UINT32 *values=v.getValues();

	for(int i=0;i<v.totalProcessCount;i++)
		os<<values[i]<<" , ";
	os<<endl;
	return os;
}

UINT32 *VectorClock::getValues() const  //取得VC这个数组的值 返回第一个地址
{
	return v;
}

VectorClock & VectorClock::operator++()  //这个线程时间戳自加
{
	v[processId]++;
	return *this;
}

VectorClock VectorClock::operator++(int)  //先拷贝了一个 然后线程时间戳自加 
{
	VectorClock tmp=*this;
	v[processId]++;
	return tmp;
}

bool VectorClock::operator<=(const VectorClock &vRight)
{
	if(operator<(vRight) || operator==(vRight))
		return true;
	return false;
}

bool VectorClock::operator<(const VectorClock &vRight)  //两个VC比较每个conpoment的时间戳
{
	bool strictlySmaller=false;
	UINT32 *vRightValues=vRight.getValues();
	for(int i=0;i<totalProcessCount;i++) {
		if(v[i]>vRightValues[i])
			return false;
		else if(v[i]<vRightValues[i]) //at least one smaller 
			strictlySmaller=true;
	}
	return strictlySmaller;
}

bool VectorClock::happensBefore(VectorClock *input)
{
	return *this < *input;
}

/**
 * is only one processId
 */
bool VectorClock::isUniqueValue(int processId)
{
	for(int i=0;i<totalProcessCount;i++)
		if(v[i]>0 && i!=processId)
			return false;
	return true;
}

bool VectorClock::areConcurrent(VectorClock *input)
{
	return (!this->happensBefore(input) && !input->happensBefore(this));
}

bool VectorClock::operator==(const VectorClock &vRight)
{
	UINT32 *vRightValues=vRight.getValues();
	for(int i=0;i<totalProcessCount;i++)
		if(v[i]!=vRightValues[i])
			return false;
	return true;
}


VectorClock::VectorClock(VectorClock *inClockPtr,int processId) //把一个VC拷贝给另一个？
{
	this->processId=processId;
	size_t size_of_bytes=sizeof(int)*totalProcessCount;
	v=(UINT32*)malloc(size_of_bytes);
	bzero(v,size_of_bytes);
	for(int i=0;i<totalProcessCount;i++)
		v[i]=inClockPtr->v[i];
}

VectorClock::VectorClock(int processId)
{
	this->processId=processId;
	size_t size_of_bytes=sizeof(int)*totalProcessCount; //地址线
	v=(UINT32*)malloc(size_of_bytes);//malloc 向系统申请分配指定size个字节的内存空间。返回类型是 void* 类型
	bzero(v,size_of_bytes);//设置为0
	if(processId!=NON_THREAD_VECTOR_CLOCK)
		v[processId]=1;
}

VectorClock::~VectorClock()
{
	free(v);
}

void VectorClock::sendEvent()
{
	event();
}

/**
 * update from the received vector clock
 */
void VectorClock::receiveAction(VectorClock *vectorClockReceived)
{
	UINT32 *vOfReceIvedClock = vectorClockReceived->getValues();
	for(int i=0;i<totalProcessCount;i++)
		v[i]=(v[i]>vOfReceIvedClock[i])?v[i]:vOfReceIvedClock[i];//谁大取谁  更新
}


void VectorClock::receiveActionFromSpecialPoint(VectorClock *vectorClockReceived,UINT32 specialPoint) 
{
	UINT32 *vOfReceIvedClock=vectorClockReceived->getValues();
	v[specialPoint]=vOfReceIvedClock[specialPoint];
}

