/*
 * CNetWorkFrameWork.h
 *
 *  Created on: 2011-11-14
 *      Author: toney
 */

#ifndef CNETWORKFRAMEWORK_H_
#define CNETWORKFRAMEWORK_H_
#include "framework/application.h"
#include "utility/singleton.h"
#include "framework/logger.h"

class CFrameWork : public AutoDeleteSingleton<CFrameWork>
{
public:
    CFrameWork();
    ~CFrameWork();

	void Run();
	void ReloadConfig()
	{
		CApplication::Instance().ConfigurationChanged();
	}

    enum RunStatus
    {
        RStatus_Back,
        RStatus_Front,
        RStatus_GenCronSh,
        RStatus_GenAndInstallCronSh,
    };

    void InitializeEnvironment(int argc, char * argv[]);

    static void ReloadConfig(int iSig);
    static void StopRun(int iSig);

    void SetServerID(uint32 svrID){
        CApplication::Instance().SetServerID(svrID);
    }
    uint32 GetServerID(){
        return CApplication::Instance().GetServerID();
    }
private:
    void ParseInputParam(int argc, char * argv[]);
    void DoFinishRun();
    void LoadConfig();

    void SetNormalLogConf(string name,int level,int MaxFileLen,int delDays);
    void UpdateLog();

    std::string GetExeFileNameWithNoPath();
    std::string GetExeFileNameWithNoExsten();
    std::string GenCheckRunSh();
    std::string AddToCrontab();
	void		WritePidToFile();

    RunStatus         m_enRunStatus;
    unsigned int      m_uiMaxFd;
    svrlib::stLogInfo m_LogInfo;

};
extern bool     g_IsStop;
extern string   g_strConfFilename;


#endif /* CNETWORKFRAMEWORK_H_ */
