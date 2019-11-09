
#ifndef _ROBOT_OPER_MGR_H__
#define _ROBOT_OPER_MGR_H__


#include "svrlib.h"


using namespace std;
using namespace svrlib;

#define MAX_SAVE_TABLE_COUNT 512 

class CGameCarcityTable;

class CRobotOperMgr : public AutoDeleteSingleton<CRobotOperMgr>
{
public:
	CRobotOperMgr() {}
	~CRobotOperMgr() {}

	void Init();
	void OnTickRobot();
	void ShutDown();
	bool PushTable(CGameCarcityTable *pTable);

private:
	CGameCarcityTable* m_arrTable[MAX_SAVE_TABLE_COUNT];
	int m_iTableCount;
};


#endif //_ROBOT_OPER_MGR_H__

