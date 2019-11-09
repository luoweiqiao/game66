/*----------------- timeFunction.h
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2012/2/2 10:47:11
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#ifndef __TIME_FUNCTION_H__
#define __TIME_FUNCTION_H__

#include "utility/basicTypes.h"
#include <time.h>
#ifdef WIN32
#include <winsock2.h>//未放置网络定义冲突
#include <windows.h>
#else //WIN32
#include <sys/time.h>
#endif // WIN32
/*************************************************************/
//--- 获得系统启动毫秒
extern uint32	getTickCount	();
//--- 获得系统启动毫秒(64位)
extern uint64	getTickCount64	();
//--- 获得系统启动毫秒(64位)【需要手动更新】
extern uint64	getSystemTick64	();
//--- 设置系统启动毫秒
extern uint64	setSystemTick64	();

//--- 获得系统秒时间
extern uint64	getSecond		();
//--- 获得系统毫秒时间
extern uint64	getMillisecond	();
//--- 获得系统微秒时间
extern uint64	getMicroseconds	();
//--- 获得时钟时间(毫秒)
extern uint64	getClockTime	();

//--- 获得系统时间
extern uint64	getTime			();
//--- 获得系统时间
extern uint64	setSysTime		();
extern uint64	getSysTime		();
//--- 获得今日开始时间
extern uint64	getDayBeginTime	();

//--- 获得本地时间
extern int32	getLocalTime	(struct tm* _Tm,const uint64* _Time);
extern int32	getLocalTime	(struct tm* _Tm,uint64 _Time);

//--- 获得时间相差天数
extern int32	diffTimeDay		(uint64 _early,uint64 _late);
//--- 获得时间相差周数
extern int32	diffTimeWeek	(uint64 _early,uint64 _late);
//--- 获得时间相差月数
extern int32	diffTimeMonth	(uint64 _early,uint64 _late);
//--- 时间格式化(YYYY-MM-DD HH:MM:SS)
extern pc_str	time_format		(uint64 _time);
//--- 睡眠
extern void		dSleep			(uint32 millisecond);

#ifndef WIN32
struct SYSTEMTIME
{
	uint16 wYear;
	uint16 wMonth;
	uint16 wDayOfWeek;
	uint16 wDay;
	uint16 wHour;
	uint16 wMinute;
	uint16 wSecond;
	uint16 wMilliseconds;
};
#endif // WIN32
extern void	getLocalTime(SYSTEMTIME&systime);

#endif // __TIME_FUNCTION_H__

