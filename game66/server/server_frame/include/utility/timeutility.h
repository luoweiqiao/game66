/**********************************************************************
*   copyright     : Tencent Innovation Center
*   version       : 0.1
*   author        : peterzhao
*   create        : 2008-10-18
*   description   : time utility
***********************************************************************/

#ifndef __TIME_UTILITY_H__
#define __TIME_UTILITY_H__

#include <time.h>
#include <string>


#include <stdint.h>
#include <sys/time.h>


namespace svrlib
{
/**
 * <p>time 处理的utility</p>
 * <p>2008-10-18建立</p>
 * <p>20011-11 修改</p>
 * @author  peterzhao
 */
class CTimeUtility
{
public:
	/**
		获取unix时间(以秒为单位)
		@return -1为失败，其它为结果
	*/
	static int32_t GetCurrentS();


	/**
		获取unix时间(以毫秒为单位)
		@return -1为失败，其它为结果
	*/
	static int64_t	GetCurrentMs();

	/**
		获取unix时间(以μ秒为单位)
		@return -1为失败，其它为结果
	*/
	static int64_t GetCurrentUs();

	/**
		功能:获取时间tm结构
		返回值:-1为失败, 0为成功
	*/
	static bool GetCurDateTime(struct tm& stTM);

	/**
		功能:获取时间日期字符串
		@return 是否成功
	*/
	static bool GetDateTimeString(std::string &strDateTime, time_t &tTime,bool bShort = false);

	/**
		功能:获取日期字符串
		@return false为失败, true为成功
	*/
	static bool	GetDateString(std::string &strDate, time_t &tTime,bool bShort = false);

	/**
		获取时间字符串
		@return false为失败, true为成功
	*/
	static bool	GetTimeString(std::string &strTime, bool bShort = false);

	/**
		从字符串中获取时间
		@return false 为失败，true为成功
	*/
	static bool  GetTimeFromStr(const std::string &strTime, time_t &tTime);

	/**
	 * sleep，以毫秒为单位
	 */
	static void Sleep(unsigned int msec);
	/**
	判断一个月有多少天	
	**/
	static int  GetMonthDays(int year,int month);


	
};

} ;

#endif//__TIME_UTILITY_H__
