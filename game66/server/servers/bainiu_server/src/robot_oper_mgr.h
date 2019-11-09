
#ifndef _ROBOT_OPER_MGR_H__
#define _ROBOT_OPER_MGR_H__



#include "svrlib.h"

#include <vector>

using namespace std;
using namespace svrlib;

#define MAX_SAVE_TABLE_COUNT 512 

class CGameBaiNiuTable;

class CRobotOperMgr : public AutoDeleteSingleton<CRobotOperMgr>
{
public:
	CRobotOperMgr() {}
	~CRobotOperMgr() {}
private:

private:
	CGameBaiNiuTable * m_arrTable[MAX_SAVE_TABLE_COUNT];
	int m_iTableCount;
public:
	bool PushTable(CGameBaiNiuTable * pTable)
	{
		LOG_DEBUG("main_push - m_iTableCount:%d,pTable:%p", m_iTableCount, pTable);
		if (pTable == NULL)
		{
			return false;
		}
		for (int i = 0; i < MAX_SAVE_TABLE_COUNT; i++)
		{
			if (m_arrTable[i] == NULL)
			{
				m_arrTable[i] = pTable;
				m_iTableCount++;

				return true;
			}
		}
		return false;
	}
public:
	bool Init();
	void OnTickRobot();
	void ShutDown();
};


#endif //_ROBOT_OPER_MGR_H__

