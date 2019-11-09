
#ifndef _ROBOT_OPER_MGR_H__
#define _ROBOT_OPER_MGR_H__


#include "svrlib.h"


using namespace std;
using namespace svrlib;

#define MAX_SAVE_TABLE_COUNT 512 

class CGameTwoeightbarTable;

class CRobotOperMgr : public AutoDeleteSingleton<CRobotOperMgr>
{
public:
	CRobotOperMgr() {}
	~CRobotOperMgr() {}

	void Init();
	void OnTickRobot();
	void ShutDown();
	bool PushTable(CGameTwoeightbarTable *pTable);

private:
	CGameTwoeightbarTable* m_arrTable[MAX_SAVE_TABLE_COUNT];
	int m_iTableCount;
};


#endif //_ROBOT_OPER_MGR_H__

