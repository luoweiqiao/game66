

#include "math/rand_table.h"
#include <assert.h>

using namespace std;
RandGen	g_RandGen	= RandGen((unsigned)time(NULL));


RandTable::RandTable()
{
	Reset();
}

RandTable::~RandTable()
{

}

//	添加某一个类型及其概率进入圆桌
void RandTable::AddIntoRoundTable(int32 iType, uint32 iProbability)
{
	if(iProbability == 0)
	{		
		return;
	}

	TROUNT_TABLE_ELEMENT stTableElem;
	stTableElem.iStartPos = m_iNowStartPos;
	stTableElem.iEndPos = m_iNowStartPos + iProbability;
	stTableElem.Type	= iType;
	m_vtTableElement.push_back(stTableElem);
	m_iNowStartPos += iProbability;
	m_iTypeNum++;
}

//	通过随机值获取某一个类型
int32 RandTable::GetTypeFromRoundTable(uint32 iRand)
{
	if(m_iNowStartPos == 0)
		return 0;

	if(iRand > m_iNowStartPos)
		iRand = iRand%m_iNowStartPos;

	for(uint32 i = 0; i < m_iTypeNum; i++)
	{
		TROUNT_TABLE_ELEMENT stTableElem = m_vtTableElement[i];
		if (iRand >= stTableElem.iStartPos && iRand < stTableElem.iEndPos)
		{
			return stTableElem.Type;
		}
	}
	
	return 0;	//	返回错误值
}

void RandTable::Reset()
{
	m_vtTableElement.clear();
	m_iNowStartPos = 0;
	m_iTypeNum = 0;
}


int RandGen::GetRand(int nStart, int nEnd)
{
	return (int)((nEnd-nStart)*RandDouble()) + nStart;
}
int32  RandGen::RandRange(int32 nMax,int32 nMin)
{
	if(nMax == nMin)
		return nMax;

	if(nMax < nMin)
	{
		int nTemp = nMax;
		nMax = nMin;
		nMin = nTemp;
	}	
	return ((RandUInt()% (nMax - nMin + 1)) + nMin);
}
bool   RandGen::RandRatio(uint32 uRatio,uint32 uRatioMax)
{
	if(uRatio == 0)
		return false;
	
	return (uRatio >= (uint32)RandRange(uRatioMax,0));
}

