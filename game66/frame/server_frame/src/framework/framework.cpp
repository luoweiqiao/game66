/*
 * CFrameWork.cpp
 *
 *  Created on: 2011-11-14
 *      Author: toney
 */

#include "framework.h"
#include <signal.h>
#include <iostream>
#include "configchecker.h"
#include "helper/filehelper.h"
#include "helper/helper.h"
#include "framework/cmdline.h"

#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<stdlib.h>

using namespace svrlib;
using std::vector;

bool     g_IsStop = false;
string   g_strConfFilename = "";

#define TICK_MAX_INTERVAL 10
#define TICK_MIN_INTERVAL 0
CFrameWork::CFrameWork()
{
    m_uiMaxFd 	 = 5000;
}
CFrameWork::~CFrameWork()
{
}

void CFrameWork::Run()
{    
	uint64 startTime = 0;
	uint64 endTime = 0;
	uint64 sleepTime = 0;
	while(!g_IsStop)
	{
		startTime = getTickCount64();
		CApplication::Instance().PreTick();
		CApplication::Instance().Tick();			

		endTime = getTickCount64();
		sleepTime = endTime - startTime;
		if(sleepTime > (TICK_MAX_INTERVAL - TICK_MIN_INTERVAL)){
            LOG_ERROR("preccess tick time out:%lld",sleepTime);
			sleepTime = TICK_MIN_INTERVAL;
		}else{
			sleepTime = TICK_MAX_INTERVAL - sleepTime;
		}
		CTimeUtility::Sleep(sleepTime);
	}
}
void CFrameWork::InitializeEnvironment(int argc, char * argv[])
{
    ParseInputParam(argc, argv);
	
    DoFinishRun();
	
    if (m_enRunStatus != RStatus_Front)
    {
        signal(SIGUSR2, ReloadConfig);
        signal(SIGUSR1, StopRun);
        CHelper::Init_daemon();
    }	
    LoadConfig();// 子进程开始
	
    if(m_uiMaxFd > 1024)
    {
        if(!CHelper::SetFileLimit(m_uiMaxFd))
        {
            std::ostringstream os;
            os << "set the max fd to " << m_uiMaxFd << " failed!";
            throw os.str().c_str();
        }
    }
	
	CApplication::Instance().PreInit();
    CConfigChecker::Instance().Start();
			
    bool bRet = CApplication::Instance().Initialize(); 
    if(bRet == false)
    {
    	exit(1);
    }
    else
    {
    	LOG_DEBUG("start server success....");
		WritePidToFile();
    }
}

void CFrameWork::ParseInputParam(int argc, char * argv[])
{
    m_enRunStatus = RStatus_Back;

    cmdline::parser a;  
    a.add<int>("sid", '\0', "server id",false, 1,cmdline::range(1,1000));
    a.add<int>("fd",'\0',"maxfd",false,10000,cmdline::range(1,50000));
    a.add<int>("loglv",'\0',"log level",false,0,cmdline::range(0,3));
    a.add<int>("logsize",'\0',"log file size",false,52428800);
    a.add<int>("logdays",'\0',"log retention time",false,5,cmdline::range(1,30));
    a.add<string>("logname",'\0',"log file name",false,"log");
    a.add<string>("cfg",'\0',"cfg file name",false,"");

    a.add("bf",0,"run front");
    a.add("gensh",0,"make gensh");
    a.add("cron",0,"add cron"); 
    
    bool ok=a.parse(argc, argv);
    if(!ok)
    {
        printf("%s : %s \n\r",a.error().c_str(),a.usage().c_str());
        exit(1);
        return; 
    }
    int svrid = a.get<int>("sid");
    int maxfd = a.get<int>("fd");
    int loglv = a.get<int>("loglv");
    int logsize = a.get<int>("logsize");
    int logdays = a.get<int>("logdays");
    string logName = a.get<string>("logname");
    string cfgName = a.get<string>("cfg");

    SetServerID(svrid);// frist

    SetNormalLogConf(logName,loglv,logsize,logdays);
    m_uiMaxFd = maxfd;
    g_strConfFilename = cfgName;

    bool bf = a.exist("bf");
    bool gensh = a.exist("gensh");
    bool cron  = a.exist("cron");
    if(bf){
        m_enRunStatus = RStatus_Front;
    }
    else if(gensh){
        m_enRunStatus = RStatus_GenCronSh;
    }
    else if(cron){
        m_enRunStatus = RStatus_GenAndInstallCronSh;
    } 

}

void CFrameWork::DoFinishRun()
{
    if (m_enRunStatus == RStatus_GenCronSh)
    {
        std::string strShFileName = GenCheckRunSh();
        std::string strTemp = "gen sh ok. file: " + strShFileName;
        throw strTemp.c_str();
    }
    else if (m_enRunStatus == RStatus_GenAndInstallCronSh)
    {
        std::string strShFileName = AddToCrontab();
        std::string strTemp = "add to crontab ok. sh file: " + strShFileName;
        throw strTemp.c_str();
    }
}

void CFrameWork::LoadConfig()
{
	CApplication::Instance().luaDoFile(g_strConfFilename.c_str());
	    
	UpdateLog();
}
void CFrameWork::ReloadConfig(int iSig)
{
    if(iSig == SIGUSR2)
    {
    	CApplication::Instance().luaDoFile(g_strConfFilename.c_str());
        CConfigChecker::Instance().ActivateReload();
    }
}

void CFrameWork::StopRun(int iSig)
{
    if(iSig == SIGUSR1)
    {
    	g_IsStop = true;        
        LOG_DEBUG("program exiting...");
    }
}
std::string CFrameWork::GetExeFileNameWithNoPath()
{
    std::string strExeName = CHelper::GetExeFileName();
    std::string::size_type iPos = strExeName.rfind('/');
    if (iPos != std::string::npos)
    {
        strExeName = strExeName.substr(iPos + 1);
    }
    return strExeName;
}

std::string CFrameWork::GetExeFileNameWithNoExsten()
{
    std::string strExeName = GetExeFileNameWithNoPath();
    std::string::size_type iPos = strExeName.rfind('.');
    if (iPos != std::string::npos)
    {
        strExeName = strExeName.substr(0, iPos);
    }
    return strExeName;
}

std::string CFrameWork::GenCheckRunSh()
{
    std::ostringstream oss;
    oss << "#!/bin/sh\n\n" << "WORK_DIR=\"" << CHelper::GetExeDir() << "\"\n\n"
            << "cd ${WORK_DIR} \n\n" 
			<< "pid=`cat pid.txt` \n\n"
			<< "processnum=`ps ax | awk '{ print $1 }' | grep ${pid}$  | grep -v grep | wc -l`\n\n"
            << "if [ $processnum -lt 1 ];\n" << "then\n" 
            << "sleep 1\n" << "./"
            << GetExeFileNameWithNoPath() << "\n" << "fi\n";

    std::string strShFileName = CHelper::GetExeDir();
    strShFileName += "checkrun_";
    strShFileName += GetExeFileNameWithNoExsten();
    strShFileName += ".sh";
    CFileHelper oFile(strShFileName.c_str(), CFileHelper::MOD_WRONLY_TRUNC);
    oFile.Write(0, oss.str().c_str(), oss.str().length());
    oFile.Close();
    CTimeUtility::Sleep(10);
    oss.str("");
    oss << "chmod +x " << strShFileName;

    ::system(oss.str().c_str());

    return strShFileName;
}

std::string CFrameWork::AddToCrontab()
{
    std::string strShFile = GenCheckRunSh();
    std::string strTmpFile = CHelper::GetExeDir() + "cron.txt";
    std::ostringstream oss;
    oss << "crontab -l > " << strTmpFile;

    ::system(oss.str().c_str());

    CTimeUtility::Sleep(100);
    CFileHelper oFile(strTmpFile.c_str(), CFileHelper::MOD_WRONLY_APPEND);
    if (!oFile.IsOpen())
    {
        throw "open cron.txt fail.";
    }

    oss.str("");
    oss << "# check run " << GetExeFileNameWithNoPath() << "\n"
            << "*/1 * * * * " << strShFile << "  >/dev/null 2>&1\n"
            << "# end check run " << GetExeFileNameWithNoPath() << "\n\n";
    if (!oFile.Write((oss.str().c_str()), oss.str().length()))
    {
        throw "write to cron.txt fail.";
    }
    oFile.Close();
    CTimeUtility::Sleep(100);

    oss.str("");
    oss << "crontab " << strTmpFile;

    ::system(oss.str().c_str());

    return strShFile;
}
void   CFrameWork::WritePidToFile()
{
    std::ostringstream oss;
    oss << getpid();
    std::string strShFileName = CHelper::GetExeDir();
    strShFileName = CStringUtility::FormatToString("pid_%d.txt",GetServerID());

    CFileHelper oFile(strShFileName.c_str(), CFileHelper::MOD_WRONLY_TRUNC);
    oFile.Write(0, oss.str().c_str(), oss.str().length());
    oFile.Close();
    CTimeUtility::Sleep(10);
    oss.str("");
    oss << "chmod 777 " << strShFileName;

    ::system(oss.str().c_str());   
}
void CFrameWork::SetNormalLogConf(string name,int level,int MaxFileLen,int delDays)
{
    m_LogInfo.strLogFileName = CStringUtility::FormatToString("%d_%s",GetServerID(),name.c_str());

    m_LogInfo.uiMaxFileLen = MaxFileLen;
    m_LogInfo.uiDays = delDays;
    m_LogInfo.uiLogLevel = level;
    m_LogInfo.bNeedTick  = false;
}
void CFrameWork::UpdateLog()
{
    svrlib::stGetLogger::GetLogger(m_LogInfo, true);
}


