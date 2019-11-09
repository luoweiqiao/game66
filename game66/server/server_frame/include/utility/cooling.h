
#pragma once
#include "basicTypes.h"
/*************************************************************/
class CCooling
{
private:
protected:
	uint64	m_uBeginTick;	/*冷却开始时间*/ 
	uint64	m_uEndTick;		/*冷却结束时间*/ 
	uint8	m_state;		//状态
public:
	CCooling();
	virtual~CCooling(){}

public:
	void	clearCool();
	/*--->[ 获得冷却时间 ]*/
	uint32	getCoolTick();
	uint32  getPassTick();
	/*--->[ 获得总冷却时间 ]*/
	uint32	getTotalTick();
	/*--->[ 开始冷却 ]*/
	bool	beginCooling(uint32 uTick);
	bool 	isTimeOut();
	void 	setState(uint8 state);
	uint8   getState();

};