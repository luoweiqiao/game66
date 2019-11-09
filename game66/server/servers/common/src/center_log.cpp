#include <error_code.pb.h>
#include "center_log.h"
#include "common_logic.h"
#include "crypt/md5.h"
#include "json/json.h"
#include "db_struct_define.h"
#include "pb/msg_define.pb.h"
#include "redis_mgr.h"

using namespace std;
using namespace svrlib;

namespace
{
    static const string s_LogFileHead[emCENTERLOG_FILE_TYPE_MAX] = {"accounttrans","gamelog","action"};
    static const string s_LogMysqlError = "mysqlerror.txt";
};

CCenterLogMgr::CCenterLogMgr()
{
    m_svrID  = 0;
    m_pTimer = NULL;
    string tmp = "/data/centerlog/";
    m_sdir = tmp;
    if (0 != mkdir(m_sdir.c_str(), 0777))
    {
        //ERROR("mkdir %s error!!!\n",m_sdir.c_str());
    }
}
CCenterLogMgr::~CCenterLogMgr()
{
    for(uint32 i=0;i<m_vecFiles.size();++i) {
        CFileStream& file = m_vecFiles[i];
        if(file.isOpen()){
            file.flush();
            file.close();
        }
    }
    if(m_mysqlFile.isOpen()){
        m_mysqlFile.flush();
        m_mysqlFile.close();
    }
}
bool CCenterLogMgr::Init(uint16 svrID)
{
    m_svrID = svrID;
    m_vecFiles.clear();
    m_vecNames.clear();
    for(uint32 i=0;i<emCENTERLOG_FILE_TYPE_MAX;++i)
    {
        CFileStream file;
        m_vecFiles.push_back(file);
        m_vecNames.push_back("");
    }
    m_pTimer = CApplication::Instance().MallocTimer(this,1);
    m_pTimer->StartTimer(5000,5000);

    CheckTransFile();

    stringstream ssfilename;
    ssfilename << m_sdir << m_svrID << "_" << s_LogMysqlError;        
    bool bRet = m_mysqlFile.open(ssfilename.str().data(),CFileStream::_Append);
    if(bRet == false){
        LOG_ERROR("CheckLogFile::open file %s error!!!\n",ssfilename.str().data());
    }
    std::ostringstream oss;
    oss << "chmod 777 " << ssfilename.str().data();
    ::system(oss.str().c_str());
        
    return true;
}
void CCenterLogMgr::ShutDown()
{
    CApplication::Instance().FreeTimer(m_pTimer);    
}
void CCenterLogMgr::OnTimer(uint8 eventID)
{
    CheckTransFile();
}
/************************************************************************
检查是否需要新建log文件,一小时一个log文件
author 刘紫华 2012-06-04
************************************************************************/
void CCenterLogMgr::CheckTransFile()
{
    return;
    struct tm *newtime;
    char filename[128];
    time_t it1;
    time(&it1);
    newtime = localtime(&it1);

    for(uint32 i=0;i<emCENTERLOG_FILE_TYPE_MAX;++i)
    {
        //strftime(filename, 128, "_%Y-%m-%d_%H.txt",newtime);
        sprintf(filename,"_%lld.txt",(getSysTime()/SECONDS_IN_MIN));
        filename[strlen(filename)] = '\0';
        stringstream ssfilename;
        ssfilename << m_sdir << m_svrID << "_" << s_LogFileHead[i] << filename;
        CFileStream& file = m_vecFiles[i];
        if(!file.isOpen())
        {
            m_vecNames[i] = ssfilename.str();
            bool bRet = file.open(ssfilename.str().data(), CFileStream::_Append);
            if (bRet == false) {
                LOG_ERROR("CheckLogFile::open file %s error!!!\n", ssfilename.str().data());
            }
            std::ostringstream oss;
            oss << "chmod 777 " << ssfilename.str().data();
            ::system(oss.str().c_str());
        }
        if(0 != strncmp(ssfilename.str().data(),m_vecNames[i].c_str(),m_vecNames[i].length()))
        {
            if(file.isOpen()){
                file.close();
            }
            m_vecNames[i] = ssfilename.str();
            bool bRet = file.open(ssfilename.str().data(),CFileStream::_Append);
            if(bRet == false){
                LOG_ERROR("CheckLogFile::open file %s error!!!\n",ssfilename.str().data());
            }
            std::ostringstream oss;
            oss << "chmod 777 " << ssfilename.str().data();
            ::system(oss.str().c_str());
        }
    }
}

/************************************************************************
将log写到log文件中,由php写回数据库
author 刘紫华 2012-06-04
param char *logdata表数据,json字符串,字段名:值
************************************************************************/
void CCenterLogMgr::WriteLog(uint32 logFileType, const char *logdata)
{
    CRedisMgr::Instance().WriteSvrLog(logdata);  
    
    return;
    if(false == m_vecFiles[logFileType].writeLine(logdata)){
        LOG_ERROR("error %s",logdata);
    }
    m_vecFiles[logFileType].flush();
}
/**
 * param: atype 货币类型
 * param: ptype 操作类型
 * param: oldv 操作之前多少
 * param: newv 操作之后多少
 */
void CCenterLogMgr::AccountTransction(uint32 uid,uint16 atype,uint16 ptype,uint32 sptype,int64 amount,int64 oldv,int64 newv,const string& chessid){
    Json::Value logValue;
    logValue["logtype"] = "translog";
    logValue["ptime"]   = getSysTime();
    logValue["uid"]     = uid;
    logValue["atype"]   = atype;
    logValue["ptype"]   = ptype;
    logValue["sptype"]  = sptype;
    logValue["amount"]  = amount;
    logValue["oldv"]    = oldv;
    logValue["newv"]    = newv;
    logValue["chessid"] = chessid;

    WriteLog(emCENTERLOG_FILE_TYPE_NORMAL,logValue.toFastString().c_str());
}
// 离线玩家金流日志
void CCenterLogMgr::OfflineAccountTransction(uint32 uid,uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid, bool haveold, stAccountInfo & data)
{
    if(diamond != 0)
	{
        AccountTransction(uid,emACC_VALUE_DIAMOND,operType,subType,diamond,-1,-1,chessid);
    }
    if(coin != 0)
	{
		if (haveold)
		{
			AccountTransction(uid, emACC_VALUE_COIN, operType, subType, coin, data.coin, data.coin + coin, chessid);
		}
		else
		{
			AccountTransction(uid, emACC_VALUE_COIN, operType, subType, coin, -1, -1, chessid);
		}        
    }
    if(score != 0)
	{
        AccountTransction(uid,emACC_VALUE_SCORE,operType,subType,score,-1,-1,chessid);
    }
    if(ingot != 0)
	{
        AccountTransction(uid,emACC_VALUE_INGOT,operType,subType,ingot,-1,-1,chessid);
    }
    if(cvalue != 0)
	{
        AccountTransction(uid,emACC_VALUE_CVALUE,operType,subType,cvalue,-1,-1,chessid);
    }
    if(safecoin != 0)
	{
		if (haveold)
		{
			AccountTransction(uid, emACC_VALUE_COIN, operType, subType, safecoin, data.safecoin, data.safecoin + safecoin, chessid);
		}
		else
		{
			AccountTransction(uid, emACC_VALUE_SAFECOIN, operType, subType, safecoin, -1, -1, chessid);
		}
        
    }
}

// 广播日志
void CCenterLogMgr::WriteGameBroadcastLog(stGameBroadcastLog& log)
{
	Json::Value logValue;
	logValue["logtype"] = "broadcastlog";
	logValue["uid"] = log.uid;
	logValue["actType"] = log.actType;
	logValue["gameType"] = log.gameType;
	logValue["score"] = log.score;

	WriteLog(emCENTERLOG_FILE_TYPE_GAMELOG, logValue.toFastString().c_str());
	LOG_DEBUG("write gamelog:%s", logValue.toFastString().c_str());
}

// 牌局日志
void CCenterLogMgr::WriteGameBlingLog(stGameBlingLog& log)
{
    Json::Value logValue;
    logValue["logtype"]   = "gamelog";
    logValue["gid"]       = log.gameType;
	logValue["welfare"]   = log.welfare;
	logValue["welctrl"]   = log.welctrl;
    logValue["roomtype"]  = log.roomType;
    logValue["consume"]   = log.consume;
    logValue["deal"]      = log.deal;
    logValue["basescore"] = log.baseScore;
    logValue["begintime"] = log.startTime;
    logValue["endtime"]   = log.endTime;
    logValue["content"]   = log.operLog.str().data();
    logValue["tid"]       = log.tableID;
    logValue["chessid"]   = log.chessid;
    logValue["rid"]       = log.roomID;

    int64 fee = 0;
    for(uint32 i=0;i<log.users.size();++i)
    {
        stBlingUser& user = log.users[i];
        Json::Value juser;
        juser["uid"]      = user.uid;
		juser["playerType"]	  = user.playerType;
		juser["welfare"] = user.welfare;
        juser["oldv"]     = user.oldValue;
        juser["newv"]     = user.newValue;
        juser["safebox"]  = user.safeCoin;
        juser["win"]      = user.win;
        juser["fee"]      = user.fee;
        juser["land"]     = user.land;
        juser["chair"]    = user.chairid;
		juser["totalwinc"] = user.totalwinc;
		//juser["stockscore"] = user.stockscore;
		juser["gamecount"] = user.gamecount;

		juser["fish_novice_times"] = user.fish_novice_times;
		juser["fish_novice_win_socre"] = user.fish_novice_win_socre;

        fee += user.fee;
        logValue["uids"].append(juser);
    }
    logValue["fee"] = fee;

    WriteLog(emCENTERLOG_FILE_TYPE_GAMELOG,logValue.toFastString().c_str());
    //LOG_DEBUG("write gamelog:%s",logValue.toFastString().c_str());
}
// 动作日志
void    CCenterLogMgr::UserActionLog(uint32 uid,uint16 act,int64 value)
{
    Json::Value logValue;
    logValue["logtype"] = "actionlog";
    logValue["time"]   = getSysTime();
    logValue["uid"]     = uid;
    logValue["act"]     = act;
    logValue["value"]   = value;
    
    WriteLog(emCENTERLOG_FILE_TYPE_ACTION,logValue.toFastString().c_str());    
}
// 错误的SQL语句
void	CCenterLogMgr::WriteErrorMysqlLog(const string& sql)
{
    Json::Value logValue;
    string strDateTime;
    time_t tTime = getSysTime();    
    CTimeUtility::GetDateTimeString(strDateTime,tTime,false);    
    logValue["time"]   = strDateTime;
    logValue["sql"]    = sql;        

    if(false == m_mysqlFile.writeLine(logValue.toFastString().c_str())){
        LOG_ERROR("error %s",logValue.toFastString().c_str());
    }
    m_mysqlFile.flush();
}











