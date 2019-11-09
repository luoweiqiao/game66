#include "utility/timeutility.h"

namespace svrlib
{
int32_t   CTimeUtility::GetCurrentS()
{
        return time(NULL);
}

int64_t CTimeUtility::GetCurrentMs()
{
	struct timeval tv;
	struct timezone tz;
	int iRet = gettimeofday(&tv, &tz);
	if ( iRet!= 0)	return -1;
	int64_t iMilli = tv.tv_sec;
	iMilli *= 1000;
	iMilli += ( tv.tv_usec / 1000 );
	return iMilli;

}

int64_t CTimeUtility::GetCurrentUs()
{
        struct timeval tv;
        struct timezone tz;
        int iRet = gettimeofday(&tv, &tz);
        if ( iRet!= 0 ) return -1;
        int64_t iUs = tv.tv_sec;
        iUs *= 1000000;
        iUs += tv.tv_usec;
        return iUs;
}

bool CTimeUtility::GetCurDateTime(struct tm& stTM)
{
	time_t curTime = time(NULL);
	if(curTime == -1) return false;
	stTM = *localtime(&curTime);
	return true;
}


bool CTimeUtility::GetDateTimeString(std::string &strDateTime,time_t &tTime, bool bShort)
{
	time_t curTime = tTime;
	if(curTime == -1) return false;
	struct tm curTm = *localtime(&curTime);
	
	char szDateBuf[56];
	int iRet  = 0;
	if(bShort) iRet =  strftime(szDateBuf, 56, "%Y%m%d%H%M%S", &curTm);
	else iRet =  strftime(szDateBuf, 56, "%Y-%m-%d %H:%M:%S", &curTm);
	if(iRet == 0) return false;

	strDateTime = szDateBuf;
	return true;
}

bool CTimeUtility::GetDateString(std::string &strDate,time_t &tTime, bool bShort)
{
	time_t curTime = tTime;
	if(curTime == -1) return false;
	struct tm curTm = *localtime(&curTime);

	char szDateBuf[30];
	int iRet  = 0;
	if(bShort) iRet =  strftime(szDateBuf, 56, "%Y%m%d", &curTm);
	else iRet =  strftime(szDateBuf, 56, "%Y-%m-%d", &curTm);
	if(iRet == 0) return false;

	strDate = szDateBuf;
	return true;
}

bool CTimeUtility::GetTimeString(std::string &strTime, bool bShort)
{
	time_t curTime = time(NULL);
	if(curTime == -1) return false;
	struct tm curTm = *localtime(&curTime);

	char szDateBuf[30];
	int iRet  = 0;
	if(bShort) iRet =  strftime(szDateBuf, 56, "H%M%S", &curTm);
	else iRet =  strftime(szDateBuf, 56, "%H:%M:%S", &curTm);
	if(iRet == 0) return false;

	strTime = szDateBuf;
	return true;
}


bool  CTimeUtility::GetTimeFromStr(const std::string &strTime, time_t &tTime)
{
	struct tm tm;
	if(strptime(strTime.c_str(), "%Y-%m-%d %H:%M:%S", &tm) == NULL)
	{
		tTime=0;
		return false;
	}
	tTime=mktime(&tm);
	return true;
}

void CTimeUtility::Sleep(unsigned int msec)
{
	timespec tm;
	tm.tv_sec = msec / 1000;
	tm.tv_nsec = msec % 1000 * 1000000;
	nanosleep(&tm, 0);
}


int	 CTimeUtility::GetMonthDays(int year,int month)
{	
	int flag=0;	
    if((year%4==0 && year%100 != 0) || year%400 == 0 )
	{
		flag=1;  //是闰年
    }
	static int const month_normal[12] = {31,28,31,30,31,30,31,31,30,31,30,31};  
	static int const month_ruinian[12] = {31,29,31,30,31,30,31,31,30,31,30,31};	

	return flag ? month_ruinian[month] : month_normal[month];		
}



} ;

