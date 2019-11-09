
#include "robot_oper_mgr.h"
#include "game_imple_table.h"


void CRobotOperMgr::Init() {
	ZeroMemory(m_arrTable, sizeof(m_arrTable));
	m_iTableCount = 0;
}

void CRobotOperMgr::OnTickRobot() {
	for (int i = 0; i < MAX_SAVE_TABLE_COUNT; ++i)
		if (m_arrTable[i] != NULL)
			m_arrTable[i]->OnRobotTick();
}

void CRobotOperMgr::ShutDown() {
	m_iTableCount = 0;
	ZeroMemory(m_arrTable, sizeof(m_arrTable));
}

bool CRobotOperMgr::PushTable(CGameCarcityTable *pTable) {
	LOG_DEBUG("main_push - m_iTableCount:%d,pTable:%p,roomid:%d,tableid:%d",
		m_iTableCount, pTable, pTable->GetRoomID(), pTable->GetTableID());
	for (int i = 0; i < MAX_SAVE_TABLE_COUNT; ++i) {
		if (m_arrTable[i] == NULL) {
			m_arrTable[i] = pTable;
			++m_iTableCount;
			return true;
		}
	}
	return false;
}