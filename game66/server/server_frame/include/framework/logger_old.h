#ifndef LOGGER_H
#define LOGGER_H

#include <cstdio>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "utility/timeutility.h"
#include "utility/thread.h"
#include "utility/likelydef.h"
#include "fundamental/ring_buff.h"
#include "helper/fileStream.h"

namespace svrlib
{
enum
{
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_NORMAL,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_CRITIC, // 致命错误
} ;

struct stLogInfo
{
	std::string strLogFileName ;
	unsigned int uiMaxFileLen ;
	unsigned int uiDays ;	
	unsigned int uiLogLevel ;
	bool 		 bNeedTick ;

	stLogInfo()
	: uiMaxFileLen(1024 * 1024 * 50),
	uiDays(3),	
	uiLogLevel(LOG_LEVEL_DEBUG),
	bNeedTick(false)
	{
		strLogFileName = "log";
	}
};
class Logger
{
	enum
	{
		DAY_SECONDS = 60 * 60 * 24,
		DEL_LOG_DAYS = 7,
		TMPBUF_LEN = 1024 * 600,
	} ;

	//enum {MAX_BUFSIZE = 64 * 1024};
public:
	Logger(const std::string& filename, size_t uiMaxFileLen = 1024 * 1024 * 50, unsigned int uiDays = 3)
		: m_iCurDay(-1),m_oFilePre(filename)
		, m_uiMaxLen(uiMaxFileLen), m_uiLen(0), m_uiDays(uiDays), m_uiLevel(LOG_LEVEL_DEBUG), m_bNeedTick(false)
	{
		m_uiCurLevel = m_uiLevel;
	}
	
	virtual ~Logger()
	{
		if(m_fileStream.isOpen()){
			m_fileStream.close();
		}
	}
	virtual bool Write(void const* pbuf, size_t nSize, bool bForce = false)
	{
		if(unlikely(m_oFilePre.empty()))
		{
			return false ;
		}

		bool ret = false;
		time_t now = time(0);
		tm * pTime = localtime(&now);
		if(m_iCurDay != pTime->tm_mday)
		{
			m_uiLen = 0;
			DoWrite(pbuf, 0, true);
			m_iCurDay = pTime->tm_mday;
			if(m_fileStream.isOpen()){
				m_fileStream.close();
			}
			std::stringstream oss;
			oss << m_oFilePre << "." << pTime->tm_year + 1900
				<< "-" << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
				<< "-" << std::setw(2) << std::setfill('0') << pTime->tm_mday
				<< ".dat";
			m_fileStream.open(oss.str().c_str(), "ab+");
/*
			::system("rm ./log.data");
			char cmd[255] = {'\0'};
			snprintf(cmd, sizeof(cmd), "ln -s %s ./log.data", oss.str().c_str());
			::system(cmd);
			::system("chmod a+x ./log.data");
*/
			if(m_fileStream.isOpen()){
				m_fileStream.seek(0,SEEK_END);
			}
			ret = DoWrite(pbuf, nSize, bForce);
			for(unsigned int i = 0; i < DEL_LOG_DAYS; ++i){
				DelTimeOutLog(m_uiDays + i) ;
			}
		}else{
			ret = DoWrite(pbuf, nSize, bForce);
		}
		m_uiLen += nSize ;
		if(ret)
		{
			if(unlikely(m_uiLen >= m_uiMaxLen))
			{
				m_uiLen = 0;
				if(m_fileStream.isOpen()){
					m_fileStream.close();
				}
				std::stringstream oss_backname, oss_curname;
				oss_backname << m_oFilePre << "_back" << "." << pTime->tm_year + 1900
				<< "-" << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
				<< "-" << std::setw(2) << std::setfill('0') << pTime->tm_mday
				<< ".dat";

				oss_curname << m_oFilePre << "." << pTime->tm_year + 1900
				<< "-" << std::setw(2) << std::setfill('0') << pTime->tm_mon + 1
				<< "-" << std::setw(2) << std::setfill('0') << pTime->tm_mday
				<< ".dat";

				// 删除备份文件
				unlink(oss_backname.str().c_str());
				// 改名
				rename(oss_curname.str().c_str(), oss_backname.str().c_str());
				// 重新打开新的文件
				m_fileStream.open(oss_curname.str().c_str(), "ab+");
			}
		}
		return ret;
	}	
	void SetLogHeader(std::string const& strFunc, std::string const& strFile, unsigned int uiLine)
	{
		m_oFormatStream.str("") ;		
		time_t lTime = time(0);
		tm * pTime = localtime(&lTime);
		m_oFormatStream << "[" << std::setw(2) << std::setfill('0') << pTime->tm_mday << " " 
			<< std::setw(2) << std::setfill('0') << pTime->tm_hour << ":"
			<< std::setw(2) << std::setfill('0') << pTime->tm_min << ":"
			<< std::setw(2) << std::setfill('0') << pTime->tm_sec  ;
		if(likely(m_bNeedTick))
		{
			m_oFormatStream << " " << CTimeUtility::GetCurrentS() << "]" ;
		}
		else
		{
			m_oFormatStream << "]" ;
		}
		static const char logLvName[LOG_LEVEL_CRITIC + 1][16] = {" [debug] "," [warning] "," [normal] "," [error] "," [critic] " };
		m_oFormatStream << logLvName[m_uiCurLevel];
		
		std::string::size_type iPos = strFile.rfind('/') ;

		if (iPos != std::string::npos)
		{
			m_oFormatStream << "[" << strFile.substr(iPos + 1) << ":" << uiLine << "]" ;
		}
		else
		{
			m_oFormatStream << "[" << strFile << ":" << uiLine << "]" ;
		}
		
		m_oFormatStream << "[" << strFunc << "]" ;		
	}
	void SetLogInfo(unsigned int uiLevel, bool bNeedTick)
	{
		m_uiLevel = uiLevel ;
		m_bNeedTick = bNeedTick ;
	}
	void FormatWrite(unsigned int uiLevel,std::string const& strFunc, std::string const& strFile, unsigned int uiLine,char const* pFormat, ...)
	{
		m_uiCurLevel = uiLevel;
		if(m_uiCurLevel < m_uiLevel){
			return ;
		}
		SetLogHeader(strFunc,strFile,uiLine);
		
		sprintf(m_pTmpBuf, "%s [", m_oFormatStream.str().c_str()) ;
		unsigned int uiLen = strlen(m_pTmpBuf) ;
		va_list ap;
		va_start(ap, pFormat);
		vsnprintf(&m_pTmpBuf[uiLen], TMPBUF_LEN - uiLen, pFormat, ap);
		va_end(ap);
		
		uiLen = strlen(m_pTmpBuf) ;
		if(uiLen + 4 >= TMPBUF_LEN)
		{
			return ; // 再打日记就越界了。
		}
		sprintf(&m_pTmpBuf[uiLen], "]\r\n") ;
		Write(m_pTmpBuf,uiLen+3, true);
	}
private:
	void DelTimeOutLog(unsigned int uiDaysAgo)
	{
		time_t now = time(0);
		time_t iAgoDay = now - uiDaysAgo * DAY_SECONDS ;
		tm * pAgoTime = localtime(&iAgoDay);
		std::stringstream oss_agoname, oss_agobackname ;
		oss_agobackname << m_oFilePre << "_back" << "." << pAgoTime->tm_year + 1900
			<< "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mon + 1
			<< "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mday
			<< ".dat";
		unlink(oss_agobackname.str().c_str());

		oss_agoname << m_oFilePre << "." << pAgoTime->tm_year + 1900
			<< "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mon + 1
			<< "-" << std::setw(2) << std::setfill('0') << pAgoTime->tm_mday
			<< ".dat";
		unlink(oss_agoname.str().c_str());
	}

	bool DoWrite(void const * pbuf, size_t nSize, bool bForce)
	{
		bForce = bForce;
		if(likely(m_fileStream.isOpen())){
			m_fileStream.write(nSize,pbuf);
			m_fileStream.flush();
			return true;
		}
		return false;
	}
protected:
	int m_iCurDay;
	CFileStream m_fileStream;
	std::string m_oFilePre;

	char  m_pTmpBuf[TMPBUF_LEN] ;
	std::stringstream m_oStream;
	std::stringstream m_oFormatStream;
	size_t m_uiMaxLen ;
	size_t m_uiLen ;
	unsigned int m_uiDays ;
	unsigned int m_uiLevel ;
	unsigned int m_uiCurLevel ;
	bool 		 m_bNeedTick ;
};


struct stGetLogger
{
	static Logger& GetLogger(stLogInfo const& oLogInfo = stLogInfo(), bool bReOpen = false)
	{
		static Logger * pLogger = 0 ;
		if(bReOpen || pLogger == 0)
		{
			if(pLogger != 0){
				delete pLogger ;
			}
			pLogger = new Logger(oLogInfo.strLogFileName, oLogInfo.uiMaxFileLen, oLogInfo.uiDays) ;			
			pLogger->SetLogInfo(oLogInfo.uiLogLevel, oLogInfo.bNeedTick);
		}
		return *pLogger ;
	}
};

#define LOG_DEBUG(fmt, arg...) \
		stGetLogger::GetLogger().FormatWrite(svrlib::LOG_LEVEL_DEBUG,__FUNCTION__, __FILE__, __LINE__,fmt, ##arg) ; 


#define LOG_WARNING(fmt, arg...) \
	  	stGetLogger::GetLogger().FormatWrite(svrlib::LOG_LEVEL_WARNING,__FUNCTION__, __FILE__, __LINE__,fmt, ##arg) ; 


#define LOG_INFO(fmt, arg...) \
		stGetLogger::GetLogger().FormatWrite(svrlib::LOG_LEVEL_INFO,__FUNCTION__, __FILE__, __LINE__,fmt, ##arg) ; 


#define LOG_ERROR(fmt, arg...) \
		stGetLogger::GetLogger().FormatWrite(svrlib::LOG_LEVEL_ERROR,__FUNCTION__, __FILE__, __LINE__,fmt, ##arg) ; 


#define LOG_CRITIC(fmt, arg...) \
		stGetLogger::GetLogger().FormatWrite(svrlib::LOG_LEVEL_CRITIC,__FUNCTION__, __FILE__, __LINE__,fmt, ##arg) ; 

} ; // end namespace svrlib



#endif


