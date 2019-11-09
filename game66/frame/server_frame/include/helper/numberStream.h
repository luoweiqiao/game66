/*----------------- numberStream.h
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/8/20 17:37:54
*--------------------------------------------------------------
*流水号产生
*------------------------------------------------------------*/
#ifndef __NUMBER_STREAM_H__
#define __NUMBER_STREAM_H__

#include "utility/basicTypes.h"
#include "utility/timeFunction.h"
#include <time.h>
/*************************************************************/
class CNumberStream
{
protected:
	/*************************************************************/
#pragma pack(push,1)
	union _U_NUMBER
	{
		struct
		{
			uint16	_order;	//序号
			uint32	_time;	//时间
			uint16	_flag;	//标记ID[必须小于32767后15位]
		}_STREAM;

		uint64		_stream;/*流水号*/ 
	};
#pragma pack(pop)
	/*************************************************************/
protected:
	_U_NUMBER	m_uNumber;
	uint32		m_uMaxTime;			/*时间*/ 

public:
	CNumberStream(uint16 _flag = 0)
	{
		m_uMaxTime			= 0;
		m_uNumber._stream	= 0;
		setFlag(_flag);
	}
	virtual~CNumberStream(){}
public:
	inline void		setFlag(uint16 _flag)
	{
		m_uNumber._STREAM._flag	= _flag;
	}
public:
	inline uint64	lastStreamNumber()
	{
		return m_uNumber._stream;
	}
	/*----->{ 产生事件流水号 }*/ 
	inline uint64	buildStreamNumber(uint16 _flag = 0)
	{
		

		/*设置标志*/ 
		if(_flag)
			setFlag(_flag);

		m_uNumber._STREAM._time = (uint32)getTime();
		/*设置时间改变标志*/ 
		if(m_uMaxTime > m_uNumber._STREAM._time)
		{
			m_uNumber._STREAM._flag|= 0x8000;
		}
		else
		{
			m_uMaxTime = m_uNumber._STREAM._time;
			m_uNumber._STREAM._flag&= 0x7FFF;
		}

		/*编号*/ 
		m_uNumber._STREAM._order++;

		return m_uNumber._stream;
	}
};

#endif // __NUMBER_STREAM_H__

