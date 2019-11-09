/*----------------- timeFunction.cpp
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2012/2/2 10:47:35
*--------------------------------------------------------------
*
Linux在编程时使用了以下几种时间类型，
1.typedef long time_t;      //<time.h>，最常用的
2.struct tim //<sys/time.h>，最小单位是微秒
{
time_t      tv_sec;
suseconds_t tv_usec;
}
3.struct timespec //<time.h>，最小单位是纳秒
{
time_t tv_sec;
long   tv_nsec;
}
4.struct tm//<time.h>，有年月日
{
int tm_sec;
int tm_min;
int tm_hour;
int tm_mday;
int tm_mon;
int tm_year;
int tm_wday;
int tm_yda;
int tm_isdst;
#ifdef _BSD_SOURCE
long tm_gmtoff;
xonst char *tm_zone;
#endif
}

*------------------------------------------------------------*/
#include "utility/timeFunction.h"
#include "utility/stringFunctions.h"
#include <memory> 
#include <string.h>
#ifdef WIN32
#include <windows.h >
#else//WIN32
#include <unistd.h> 
#include <sys/times.h> 
#endif//WIN32
/*************************************************************/
static uint64	g_uSystemTick64 = 0;
static uint64	g_uSystemTime = 0;
//-------------------------------------------------------------
//------------------------------ 获得系统启动毫秒
uint32	getTickCount	()
{
#ifdef WIN32
	//只精确到55ms
	return ::GetTickCount();
	//只精确到10ms
	//timeGetTime();
#else//WIN32
	tms tm;
	return ::times(&tm);
#endif // WIN32
}
//-------------------------------------------------------------
//------------------------------ 获得系统启动毫秒
uint64	getTickCount64	()
{
#ifdef WIN32
	return ::GetTickCount64();
#else//WIN32
	/*struct timespec
	{
		time_t tv_sec; // seconds[秒]
		long tv_nsec; // nanoseconds[纳秒]
	};
	int clock_gettime(clockid_t clk_id, struct timespect *tp);
	//@clk_id:
	CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变
	CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
	CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
	CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
	*/

	timespec _spec;
	clock_gettime(CLOCK_MONOTONIC,&_spec);
	uint64 uTick = _spec.tv_sec * 1000 + _spec.tv_nsec / 1000 / 1000;

	/*timeval _val;
	::gettimeofday(&_val,NULL);
	uint64 uTick = _val.tv_sec * 1000 + _val.tv_usec / 1000;*/
	return uTick;
#endif // WIN32
}
//-------------------------------------------------------------
//------------------------------ 获得系统启动毫秒(64位)【需要手动更新】
uint64	getSystemTick64	()
{
	if(!g_uSystemTick64)
		setSystemTick64();

	return g_uSystemTick64;
}
//-------------------------------------------------------------
//------------------------------ 设置系统启动毫秒
uint64	setSystemTick64	()
{
	g_uSystemTick64 = getTickCount64();

	return g_uSystemTick64;
}
//-------------------------------------------------------------
//------------------------------ 获得系统秒时间
uint64	getSecond		()
{
	return ::time(NULL);
}

//-------------------------------------------------------------
//------------------------------ 获得系统毫秒时间
uint64	getMillisecond	()
{
#ifdef WIN32
	//系统启动以后的毫秒级时间
	//clock();
	//只精确到55ms
	return ::GetTickCount64();

#else//WIN32
	/*struct timespec
	{
		time_t tv_sec; // seconds[秒]
		long tv_nsec; // nanoseconds[纳秒]
	};
	int clock_gettime(clockid_t clk_id, struct timespect *tp);
	//@clk_id:
	CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变
	CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
	CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
	CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
	*/

	timespec _spec;
	clock_gettime(CLOCK_MONOTONIC,&_spec);
	uint64 uTick = _spec.tv_sec * 1000 + _spec.tv_nsec / 1000 / 1000;

	/*timeval _val;
	::gettimeofday(&_val,NULL);
	uint64 uTick = _val.tv_sec * 1000 + _val.tv_usec / 1000;*/
	return uTick;
#endif // WIN32
}
//-------------------------------------------------------------
//------------------------------ 获得系统微秒时间
uint64	getMicroseconds	()
{
#ifdef WIN32
	/*LARGE_INTEGER stFrequency;
	QueryPerformanceFrequency(&stFrequency); */
	LARGE_INTEGER stCount;
	QueryPerformanceCounter(&stCount); 

	return uint64(stCount.QuadPart);
#else//WIN32
	/*struct timespec
	{
		time_t tv_sec; // seconds[秒]
		long tv_nsec; // nanoseconds[纳秒]
	};
	int clock_gettime(clockid_t clk_id, struct timespect *tp);
	//@clk_id:
	CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变
	CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
	CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
	CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
	*/

	timespec _spec;
	clock_gettime(CLOCK_MONOTONIC,&_spec);
	uint64 uTick = _spec.tv_sec * 1000 * 1000 + _spec.tv_nsec / 1000;

	/*timeval _val;
	::gettimeofday(&_val,NULL);
	uint64 uTick = _val.tv_sec * 1000 * 1000 + _val.tv_usec;*/
	return uTick;
#endif // WIN32
}
//-------------------------------------------------------------
//------------------------------ 获得时钟时间(毫秒)
uint64	getClockTime	()
{
#ifdef WIN32
	uint64 uTick = uint64(getTime()) * 1000;
#else//WIN32
	timespec _spec;
	clock_gettime(CLOCK_REALTIME,&_spec);

	uint64 uTick = _spec.tv_sec * 1000 + _spec.tv_nsec / 1000 / 1000;
#endif // WIN32
	return uTick;
}
//-------------------------------------------------------------
//------------------------------ 获得系统时间
uint64	getTime			()
{
	return ::time(NULL);
}
//-------------------------------------------------------------
//------------------------------ 
uint64	setSysTime		()
{
	static uint64 g_uTime = 0;
	if(!g_uTime)
	{
		/*
		tm tm_temp;
		tm_temp.tm_year	= 2010-1900;//年1900+
		tm_temp.tm_mon	= 0;		//月[0,11]
		tm_temp.tm_mday	= 1;		//日[1,31]
		tm_temp.tm_hour	= 0;		//时[0,23]
		tm_temp.tm_min	= 0;		//分[0,59]
		tm_temp.tm_sec	= 0;		//秒[0,59]
		g_uTime	= mktime(&tm_temp);
		*/
	}

	g_uSystemTime = getTime();

	if(g_uSystemTime > g_uTime)
		g_uSystemTime -=g_uTime;

	//在这里可以制定一个初始时间,用来做时间片

	return g_uSystemTime;
}
//-------------------------------------------------------------
//------------------------------ 获得系统时间
uint64	getSysTime		()
{
	if(!g_uSystemTime)
		setSysTime();

	return g_uSystemTime;
}
//-------------------------------------------------------------
//------------------------------ 获得今日开始时间
uint64	getDayBeginTime	()
{
	static tm	_tm;
	memset(&_tm,0,sizeof(_tm));

	getLocalTime(&_tm,getTime());
	_tm.tm_hour= 0;
	_tm.tm_min = 0;
	_tm.tm_sec = 0;

	return mktime(&_tm);
}
//-------------------------------------------------------------
//------------------------------ 获得本地时间
int32	getLocalTime	(struct tm* _Tm,const uint64* _Time)
{
	if(!_Tm || !_Time)
		return -1;

	time_t _time = *_Time;
#ifdef WIN32
	return ::localtime_s(_Tm,&_time);
#else//WIN32
	if(_Tm)
		memset(_Tm,0xff,sizeof(struct tm));
	struct tm* tmp = ::localtime(&_time);
	if(tmp && _Tm)
		memcpy(_Tm,tmp,sizeof(struct tm));
	return 0;
#endif // WIN32
}
//-------------------------------------------------------------
//------------------------------ 获得本地时间
int32	getLocalTime	(struct tm* _Tm,uint64 _Time)
{
	if(!_Tm || !_Time)
		return -1;

	return getLocalTime(_Tm,&_Time);
}

//-------------------------------------------------------------
//------------------------------ 获得时间相差天数
int32	diffTimeDay		(uint64 _early,uint64 _late)
{
	if(_early == 0 || _late == 0)
		return 0;

	static tm	tm_early;
	static tm	tm_late;
	memset(&tm_early,0,sizeof(tm_early));
	memset(&tm_late,0,sizeof(tm_late));

	getLocalTime(&tm_early,_early);
	getLocalTime(&tm_late,_late);

	if(tm_early.tm_year > tm_late.tm_year)
		return 0;

	//同年同日
	if(tm_early.tm_year == tm_late.tm_year && tm_early.tm_yday == tm_late.tm_yday)
		return 0;

	//同年判断
	if(tm_early.tm_year == tm_late.tm_year)
	{
		if(tm_early.tm_yday >= tm_late.tm_yday)
			return 0;

		return (tm_late.tm_yday - tm_early.tm_yday);
	}

	int32 iDay = 0;
	//不同年时
	if(tm_early.tm_year != tm_late.tm_year)
	{
		tm tm_temp = tm_early;

		//获取12月31日时间
		tm_temp.tm_mon	= 11;
		tm_temp.tm_mday = 31;
		tm_temp.tm_yday = 0;
		uint64 _temp	= mktime(&tm_temp);

		getLocalTime(&tm_temp,_temp);
		iDay = tm_temp.tm_yday - tm_early.tm_yday;

		iDay+=1;//跨年+1

		//获得相差年天数
		for (int32 i = tm_early.tm_year + 1;i < tm_late.tm_year;i++)
		{
			tm_temp.tm_year++;
			tm_temp.tm_yday = 0;

			_temp	= mktime(&tm_temp);
			getLocalTime(&tm_temp,_temp);

			iDay += tm_temp.tm_yday;
			iDay+=1;//跨年+1
		}
	}

	return (iDay + tm_late.tm_yday);
}
//-------------------------------------------------------------
//------------------------------ 获得时间相差周数
int32	diffTimeWeek	(uint64 _early,uint64 _late)
{
	if(_early == 0 || _late == 0)
		return 0;

	static tm	tm_early;
	static tm	tm_late;
	memset(&tm_early,0,sizeof(tm_early));
	memset(&tm_late,0,sizeof(tm_late));

	getLocalTime(&tm_early,_early);
	getLocalTime(&tm_late,_late);

	if(tm_early.tm_year > tm_late.tm_year)
		return 0;

	//同年同日
	if(tm_early.tm_year == tm_late.tm_year && tm_early.tm_yday == tm_late.tm_yday)
		return 0;

	//计算两个日期的每一个周六相差多少天
	if(tm_early.tm_wday != 6)
		tm_early.tm_mday +=(6 - tm_early.tm_wday);
	if(tm_late.tm_wday != 6)
		tm_late.tm_mday +=(6 - tm_late.tm_wday);

	int32 iDay = diffTimeDay(mktime(&tm_early),mktime(&tm_late));

	int32 iWeek = 0;
	if(iDay > 0)
		iWeek = iDay / 7;//肯定相差都是7的倍数因为都是周六

	return iWeek;
}
//-------------------------------------------------------------
//------------------------------ 获得时间相差月数
int32	diffTimeMonth	(uint64 _early,uint64 _late)
{
	if(_early == 0 || _late == 0)
		return 0;

	static tm	tm_early;
	static tm	tm_late;
	memset(&tm_early,0,sizeof(tm_early));
	memset(&tm_late,0,sizeof(tm_late));

	getLocalTime(&tm_early,_early);
	getLocalTime(&tm_late,_late);

	if(tm_early.tm_year > tm_late.tm_year)
		return 0;

	//同年同月
	if(tm_early.tm_year == tm_late.tm_year && tm_early.tm_mon == tm_late.tm_mon)
		return 0;

	//同年判断
	if(tm_early.tm_year == tm_late.tm_year)
		return (tm_late.tm_mon - tm_early.tm_mon);

	int32 iMon = 0;
	//不同年时
	if(tm_early.tm_year != tm_late.tm_year)
	{
		//计算相差年数
		iMon = (tm_late.tm_year - tm_early.tm_year) * 12;
		//再计算相差月数
		iMon += tm_late.tm_mon;
		if(iMon >= tm_early.tm_mon)
			iMon -= tm_early.tm_mon;
		else
			iMon = 0;
	}

	return iMon;
}

//-------------------------------------------------------------
//------------------------------ 时间格式化(YYYY-MM-DD HH:MM:SS)
pc_str	time_format		(uint64 _time)
{
	tm	tmTime;
	memset(&tmTime,0,sizeof(tmTime));
	if(getLocalTime(&tmTime,_time) != 0)
		return "";

	static char szDate[32] = {0};
	dSprintf(szDate,sizeof(szDate),"%.4d-%.2d-%.2d %.2d:%.2d:%.2d",tmTime.tm_year + 1900,tmTime.tm_mon + 1,tmTime.tm_mday,tmTime.tm_hour,tmTime.tm_min,tmTime.tm_sec);

	return szDate;
}

//-------------------------------------------------------------
//------------------------------ 睡眠
void	dSleep	(uint32 millisecond)
{
#ifdef WIN32
	::Sleep(millisecond);
#else//WIN32
	usleep( millisecond * 1000);
#endif // WIN32
}
//-------------------------------------------------------------
//------------------------------ 
void	getLocalTime(SYSTEMTIME&systime)
{
#ifdef WIN32
	::GetLocalTime(&systime);
#else//WIN32
	memset(&systime,0,sizeof(systime));
	time_t _time = getTime();
	struct tm* tmp = ::localtime(&_time);
	if(tmp)
	{
		timespec _spec;
		clock_gettime(CLOCK_REALTIME,&_spec);

		systime.wYear			= tmp->tm_year + 1900;
		systime.wMonth			= tmp->tm_mon + 1;
		systime.wDayOfWeek		= tmp->tm_wday;
		systime.wDay			= tmp->tm_mday;
		systime.wHour			= tmp->tm_hour;
		systime.wMinute			= tmp->tm_min;
		systime.wSecond			= tmp->tm_sec;
		systime.wMilliseconds	= _spec.tv_nsec / 1000 / 1000;
	}
#endif // WIN32
}



