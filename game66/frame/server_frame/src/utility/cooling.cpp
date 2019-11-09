
#include "utility/timeFunction.h"
#include "utility/cooling.h"
/*************************************************************/
//-------------------------------------------------------------
//------------------------------ 
CCooling::CCooling()
{
	clearCool();
	m_state		= 0;
}

//-------------------------------------------------------------
//------------------------------ 
void	CCooling::clearCool	()
{
	m_uBeginTick= 0;
	m_uEndTick	= 0;
}

//-------------------------------------------------------------
//------------------------------ 
uint32	CCooling::getCoolTick()
{
	if(!m_uBeginTick || !m_uEndTick)
		return 0;

	uint64 uTick = getSystemTick64();
	return (m_uEndTick > uTick ? uint32(m_uEndTick - uTick) : 0);
}
uint32  CCooling::getPassTick()
{
	if(!m_uBeginTick || !m_uEndTick)
		return 0;

	uint64 uTick = getSystemTick64();
	if(uTick > m_uBeginTick)
		return (uint32(uTick - m_uBeginTick));
	return 0;
}
//-------------------------------------------------------------
//------------------------------ 获得总冷却时间
uint32	CCooling::getTotalTick()
{
	return ((m_uEndTick > m_uBeginTick) ? uint32(m_uEndTick - m_uBeginTick) : 0);
}

//-------------------------------------------------------------
//------------------------------ 
bool	CCooling::beginCooling(uint32 uTick)
{
	if(uTick <= 0)
	{
		m_uBeginTick= 0;
		m_uEndTick	= 0;
		return false;
	}
	m_uBeginTick= getSystemTick64();
	m_uEndTick	= m_uBeginTick + uTick;

	return true;
}
bool 	CCooling::isTimeOut()
{
	return getCoolTick() == 0;
}
void 	CCooling::setState(uint8 state)
{
	m_state = state;
}
uint8   CCooling::getState()
{
	return m_state;
}


















