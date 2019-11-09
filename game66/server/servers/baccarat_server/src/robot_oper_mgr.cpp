
#include "robot_oper_mgr.h"

#include "game_imple_table.h"


bool CRobotOperMgr::Init()
{
	for (int i = 0; i < MAX_SAVE_TABLE_COUNT; i++)
	{
		m_arrTable[i] = NULL;
	}
	m_iTableCount = 0;
	return true;
}



void CRobotOperMgr::OnTickRobot()
{
	//LOG_DEBUG("main_loop - m_iTableCount:%d", m_iTableCount);
	for (int i = 0; i < MAX_SAVE_TABLE_COUNT; i++)
	{
		if (m_arrTable[i] != NULL)
		{
			//LOG_DEBUG("main_loop_sub - m_iTableCount:%d", m_iTableCount);
			m_arrTable[i]->OnRobotTick();
		}
	}
}

void CRobotOperMgr::ShutDown()
{
	m_iTableCount = 0;
	for (int i = 0; i < MAX_SAVE_TABLE_COUNT; i++)
	{
		m_arrTable[i] = NULL;
	}
}