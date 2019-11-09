

#include "dbmysql_mgr.h"
#include "svrlib.h"
#include <string.h>
#include "common_logic.h"
#include "json/json.h"
#include "player_mgr.h"
#include "data_cfg_mgr.h"
#include "center_log.h"
#include "msg_define.pb.h"

using namespace std;
using namespace svrlib;

namespace
{
	static const string gametables[net::GAME_CATE_MAX_TYPE] = { "","game_land","game_showhand","game_bainiu","game_texas","game_zajinhua","game_niuniu","game_baccarat","game_sangong","game_paijiu","game_everycolor","game_dice","game_two_people_majiang","game_slot", "game_war","game_fight", "game_robniu", "game_fishing", "game_twoeight", "game_carcity" };
};
void CDBTaskImple::writeLog(string logStr)
{
    CCenterLogMgr::Instance().WriteErrorMysqlLog(logStr);    
}
CDBMysqlMgr::CDBMysqlMgr()
{
    m_pAsyncDBCallBack = NULL;
    m_svrID = 0;
    m_pReportTimer = NULL;
}
CDBMysqlMgr::~CDBMysqlMgr()
{

}
void  CDBMysqlMgr::OnTimer(uint8 eventID)
{
    switch(eventID)
    {
    case 1:
        ReportOnlines();
        break;
    default:
        break;
    }
}
bool  CDBMysqlMgr::Init(stDBConf szDBConf[])
{
    for(int32 i=0;i<DB_INDEX_TYPE_MAX;++i)
    {
		LOG_DEBUG("mysql_connect_cofig - i:%d,uPort:%d,sHost:%s,sUser:%s,sPwd:%s,sDBName:%s",
			i,szDBConf[i].uPort, szDBConf[i].sHost.c_str(), szDBConf[i].sUser.c_str(), szDBConf[i].sPwd.c_str(), szDBConf[i].sDBName.c_str());
        m_DBConf[i] = szDBConf[i];
    }
	if(!ConnectSyncDB()){
		return false;
	}
	
	StartAsyncDB();
	
    m_svrID = CApplication::Instance().GetServerID();
    m_pReportTimer = CApplication::Instance().MallocTimer(this,1);
    m_pReportTimer->StartTimer(3000,20000);//20秒上报一次在线
	return true;
}
void	CDBMysqlMgr::ShutDown()
{
	for(int i=0;i<DB_INDEX_TYPE_MAX;++i)
	{
		if(m_pAsyncTask[i] != NULL)
		{
			m_pAsyncTask[i]->stop();
			m_pAsyncTask[i]->join();
			
			delete m_pAsyncTask[i];
			m_pAsyncTask[i] = NULL;
		}
        m_syncDBOper[i].dbClose();
	}	
    CApplication::Instance().FreeTimer(m_pReportTimer);
}
void CDBMysqlMgr::SetAsyncDBCallBack(AsyncDBCallBack* pCallBack)
{
    m_pAsyncDBCallBack = pCallBack;
}
void CDBMysqlMgr::ProcessDBEvent()
{
    uint32 num = 0;
    for(int i=0;i<DB_INDEX_TYPE_MAX;++i)
    {
        if(m_pAsyncTask[i] == NULL)
            continue;
        CDBEventRep* pRep = m_pAsyncTask[i]->GetAsyncQueryResult();
        while (pRep != NULL)
        {
            OnProcessDBEvent(pRep);
            m_pAsyncTask[i]->FreeDBEventRep(pRep);
            if (++num >= 200)
            {
                pRep = NULL;
                break;
            }
            pRep = m_pAsyncTask[i]->GetAsyncQueryResult();
        }
    }
}
// 启动异步线程
bool CDBMysqlMgr::StartAsyncDB()
{
    for(int i=0;i<DB_INDEX_TYPE_MAX;++i)
    {
        m_pAsyncTask[i] = new CDBTaskImple();
        m_pAsyncTask[i]->setDBIndex(i);
		m_pAsyncTask[i]->init(m_DBConf[i]);
        m_pAsyncTask[i]->start();
    }

    return true;
}
// 连接配置服务器
bool	CDBMysqlMgr::ConnectSyncDB()
{
    for(int i =0;i<DB_INDEX_TYPE_MAX;++i){
    	stDBConf& refConf = m_DBConf[i];
    	bool bRet = m_syncDBOper[i].dbOpen(refConf.sHost.c_str(),refConf.sUser.c_str(),refConf.sPwd.c_str(),refConf.sDBName.c_str(),refConf.uPort);
    	if(bRet == false){
    		LOG_ERROR("连接配置数据库失败:");
    		return false;
    	}
    }
    LOG_ERROR("连接配置数据库成功");
	return true;
}
void CDBMysqlMgr::AddAsyncSql(uint8 dbType,string strSql)
{
	if(dbType >= DB_INDEX_TYPE_MAX)
	{
		LOG_ERROR("数据库类型越界:%d",dbType);
		return;
	}
	m_pAsyncTask[dbType]->push(strSql);
}
// 更新服务器配置信息
void  CDBMysqlMgr::UpdateServerInfo()
{
    LOG_DEBUG("update server info");
    string svrip,svrlanip;
    svrlanip = CHelper::GetLanIP();
    if(CHelper::IsHaveNetIP())
    {
        svrip = CHelper::GetNetIP();
        ZeroSqlBuff();
        sprintf(m_szSql,"UPDATE serverinfo SET svrip='%s',svrlanip='%s' WHERE svrid=%u;",svrip.c_str(),svrlanip.c_str(),m_svrID);
        SendCommonLog(DB_INDEX_TYPE_ACC);
    }else{//有可能没有公网ip,手动配置
        ZeroSqlBuff();
        sprintf(m_szSql, "UPDATE serverinfo SET svrlanip='%s' WHERE svrid=%u;",svrlanip.c_str(), m_svrID);
        SendCommonLog(DB_INDEX_TYPE_ACC);
    }
}

// 上报服务器在线人数
void  CDBMysqlMgr::ReportOnlines()
{
    uint32 onlines = 0,robots = 0;
    CPlayerMgr::Instance().GetOnlines(onlines,robots);
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE serverinfo SET onlines=%d,robots=%d, report_time = NOW() WHERE svrid =%u;",onlines,robots,m_svrID);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}

// 更新玩家在线服务器房间
void    CDBMysqlMgr::UpdatePlayerOnlineInfo(uint32 uid,uint32 svrid,uint32 roomid,uint8 playerType,int64 coin,int64 safecoin,int64 score,string strCity)
{
    ZeroSqlBuff();
    if(svrid != 0)
    {
        if(uid >= ROBOT_MGR_ID)return;
        if(coin != 0 || safecoin != 0 || score != 0 || strcmp(strCity.c_str(),"")!=0){
            sprintf(m_szSql,"insert into onlinedetail set uid=%d,svrid=%d,roomid=%d,isrobot=%d,coin=%lld,safecoin=%lld,score=%lld,city='%s' ON DUPLICATE KEY UPDATE svrid=%d,roomid=%d,coin=%lld,safecoin=%lld,score=%lld,city='%s';"
                ,uid,svrid,roomid,playerType,coin,safecoin,score, strCity.c_str(),svrid,roomid,coin,safecoin,score, strCity.c_str());
        }else{
            sprintf(m_szSql,"insert into onlinedetail set uid=%d,svrid=%d,roomid=%d,isrobot=%d ON DUPLICATE KEY UPDATE svrid=%d,roomid=%d;",uid,svrid,roomid,playerType,svrid,roomid);
        }
    }else{
        sprintf(m_szSql,"delete from onlinedetail where uid=%d;",uid);
    }
	//LOG_DEBUG("m_szSql:%s", m_szSql);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
void    CDBMysqlMgr::UpdatePlayerOnlineInfoEx(uint32 uid, uint32 svrid, uint32 roomid, uint8 playerType, int64 coin, int64 safecoin, int64 score, string strCity)
{
	ZeroSqlBuff();
	sprintf(m_szSql, "insert into onlinedetail set uid=%d,svrid=%d,roomid=%d,isrobot=%d,coin=%lld,safecoin=%lld,score=%lld,city='%s' ON DUPLICATE KEY UPDATE svrid=%d,roomid=%d,coin=%lld,safecoin=%lld,score=%lld,city='%s';", uid, svrid, roomid, playerType, coin, safecoin, score, strCity.c_str(), svrid, roomid, coin, safecoin, score, strCity.c_str());
	LOG_DEBUG("m_szSql:%s", m_szSql);
	SendCommonLog(DB_INDEX_TYPE_ACC);
}
void    CDBMysqlMgr::ClearPlayerOnlineInfo(uint32 svrid)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"delete from onlinedetail where svrid=%d;",svrid);    
    SendCommonLog(DB_INDEX_TYPE_ACC);  
}

// 更新连续登陆奖励
void    CDBMysqlMgr::UpdatePlayerLoginInfo(uint32 uid,uint32 offlinetime,uint32 clogin,uint32 weeklogin,uint32 reward,uint32 bankrupt,uint32 dgameCount)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE user%d SET offlinetime=%d,clogin=%d,weeklogin=%d,reward=%d,bankrupt=%d,dgcount=%d WHERE uid=%u;",\
            CCommonLogic::GetDataTableNum(uid),offlinetime,clogin,weeklogin,reward,bankrupt,dgameCount,uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}

void    CDBMysqlMgr::UpdatePlayerClogin()
{
	ZeroSqlBuff();
	sprintf(m_szSql, "UPDATE user0 SET clogin=0,reward=0;");
	SendCommonLog(DB_INDEX_TYPE_ACC);
}

void    CDBMysqlMgr::UpdatePlayerLoginTime(uint32 uid,uint32 logintime,string loginip)
{
    ZeroSqlBuff();
    //sprintf(m_szSql,"UPDATE user%d SET logintime=%d,ip='%s' WHERE uid=%u;",CCommonLogic::GetDataTableNum(uid),logintime,loginip.c_str(),uid);
	sprintf(m_szSql, "UPDATE user%d SET logintime=%d WHERE uid=%u;", CCommonLogic::GetDataTableNum(uid), logintime, uid);

    SendCommonLog(DB_INDEX_TYPE_ACC);
}
void    CDBMysqlMgr::AddPlayerLoginTimeInfo(uint32 uid,uint32 days,uint32 playTime)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE user%d SET alllogin=alllogin+%d,onlinetime=onlinetime+%d WHERE uid=%u;",\
            CCommonLogic::GetDataTableNum(uid),days,playTime,uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);      
}
// 设置玩家保险箱密码
void    CDBMysqlMgr::UpdatePlayerSafePasswd(uint32 uid,string passwd)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE user%d SET safepwd = '%s' WHERE uid =%u;",CCommonLogic::GetDataTableNum(uid),passwd.c_str(),uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 修改玩家账号数值（增量修改）
bool    CDBMysqlMgr::ChangeAccountValue(uint32 uid,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE account%d SET diamond=diamond + %lld,coin=coin + %lld,ingot=ingot + %lld,score=score + %lld,cvalue=cvalue + %d,safecoin=safecoin + %lld where uid=%d;",
	CCommonLogic::GetDataTableNum(uid),diamond,coin,ingot,score,cvalue,safecoin,uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
	int row = 1;
	LOG_DEBUG("uid:%d,coin:%d,row:%d", uid, coin, row);
	return row > 0;
}

bool    CDBMysqlMgr::AtomChangeAccountValue(uint32 uid,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin)
{
    uint32 row = m_syncDBOper[DB_INDEX_TYPE_ACC].GetAffectedNumExeSqlCmd("UPDATE account%d SET diamond=diamond+%lld,coin=coin+%lld,ingot=ingot+%lld,score=score+%lld,cvalue=cvalue+%d,safecoin=safecoin+%lld where uid=%d \
    and (diamond + %lld) >= 0 and (coin + %lld) >= 0 and (ingot + %lld) >= 0 and (score + %lld) >= 0 and (cvalue + %d) >= 0 and (safecoin + %lld) >= 0;",
    CCommonLogic::GetDataTableNum(uid),diamond,coin,ingot,score,cvalue,safecoin,uid,diamond,coin,ingot,score,cvalue,safecoin);

	LOG_DEBUG("uid:%d,coin:%d,row:%d", uid, coin, row);
    return row > 0;
}


void    CDBMysqlMgr::ChangeFeeValue(uint32 uid,int64 feewin,int64 feelose)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE account%d SET feewin=feewin + %lld,feelose=feelose + %lld where uid=%d;",
    CCommonLogic::GetDataTableNum(uid),feewin,feelose,uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);    
}
// 增加玩家破产次数
void    CDBMysqlMgr::AddBankruptValue(uint32 uid)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE account%d SET bankrupt=bankrupt+1 where uid=%d;",CCommonLogic::GetDataTableNum(uid),uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 增加机器人登陆次数
void    CDBMysqlMgr::AddRobotLoginCount(uint32 uid)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE robot SET logincount=logincount+1 where uid=%d;",uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
void    CDBMysqlMgr::SetRobotLoginState(uint32 uid,uint8 state)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE robot SET loginstate=%d where uid=%d;",state,uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 清空机器人登陆状态
void    CDBMysqlMgr::ClearRobotLoginState()
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE robot SET logincount=0,loginstate=0;");
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 保险箱赠送操作
void    CDBMysqlMgr::GiveSafeBox(uint32 suid,uint32 ruid,int64 coin,int64 tax,string strIp)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"insert into scoinrecord (suid,ruid,amount,tax,ptime,ip) values(%d,%d,%lld,%lld,%lld,'%s');",suid,ruid,coin,tax,getSysTime(),strIp.c_str());
    SendCommonLog(DB_INDEX_TYPE_ACC);

    ZeroSqlBuff();
    sprintf(m_szSql,"CALL p_update_scoinrecord(%d,%d);",suid,ruid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 插入游戏数据记录
void    CDBMysqlMgr::InsertGameValue(uint16 gameType,uint32 uid)
{
	LOG_DEBUG("gameType:%d,uid:%d", gameType, uid);
    ZeroSqlBuff();
    sprintf(m_szSql,"insert into %s (uid) values(%d);",gametables[gameType].c_str(),uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);         
}
// 修改游戏数据记录
// win lose 输赢局数
/*
void    CDBMysqlMgr::ChangeGameValue(uint16 gameType,uint32 uid,bool isCoin,int32 win,int32 lose,int64 winscore, int64 lExWinScore,int64 maxscore, string strbet,string strwin)
{
	
    ZeroSqlBuff();
	if (gameType == net::GAME_CATE_TEXAS || gameType == net::GAME_CATE_SHOWHAND )
	{
		if (isCoin)
		{
			sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,totalwinc=totalwinc+%lld,maxwinc=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, winscore, maxscore, uid);
		}
	}
	else if (gameType == net::GAME_CATE_FRUIT_MACHINE)
	{
		if (isCoin)
		{
			if (lExWinScore <= 0)
			{
				sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld,gamecount=gamecount+%d,vecbet='%s',vecwin='%s' where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, 1, strbet.c_str(),strwin.c_str(), uid);
			}
			else
			{
				sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld,gamecount=%d,vecbet='%s',vecwin='%s' where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, 0, strbet.c_str(),strwin.c_str(), uid);
			}
		}
	}
	else
	{
		if (gameType != net::GAME_CATE_EVERYCOLOR)
		{
			if (gameType == net::GAME_CATE_BULLFIGHT || gameType == net::GAME_CATE_BACCARAT || gameType == net::GAME_CATE_PAIJIU || gameType == net::GAME_CATE_DICE || gameType == net::GAME_CATE_WAR)
			{
				sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld,vecbet='%s',vecwin='%s' where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore,strbet.c_str(), strwin.c_str(), uid);
			}
			else
			{
				if (isCoin)
				{
					sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, uid);
				}
				else
				{
					sprintf(m_szSql, "UPDATE %s set win=win+%d,lose=lose+%d,daywin=daywin+%lld,maxwin=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, uid);
				}
			}
		}
	}
	LOG_DEBUG("gameType:%d,uid:%d,m_szSql:%s", gameType, uid, m_szSql);
    SendCommonLog(DB_INDEX_TYPE_ACC);
    //增加累计输赢
    if(isCoin)
	{
        ZeroSqlBuff();
        sprintf(m_szSql,"UPDATE account%d SET win=win+%lld where uid=%d;",CCommonLogic::GetDataTableNum(uid),winscore,uid);
        SendCommonLog(DB_INDEX_TYPE_ACC);
    }
    
}
*/
void    CDBMysqlMgr::ChangeGameValue(uint16 gameType, uint32 uid, bool bIsRobot, bool isCoin, int32 win, int32 lose, int64 winscore, int64 lExWinScore, int64 maxscore,int rwelfare,int welcount)
{
	ZeroSqlBuff();
	if (gameType == net::GAME_CATE_TEXAS || gameType == net::GAME_CATE_SHOWHAND)
	{
		if (isCoin)
		{
			sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,totalwinc=totalwinc+%lld,maxwinc=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, winscore, maxscore, uid);
		}
	}
	else if (gameType == net::GAME_CATE_FRUIT_MACHINE)
	{
		if (isCoin)
		{
			if (lExWinScore <= 0)
			{
				sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld,gamecount=gamecount+%d where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, 1, uid);
			}
			else
			{
				sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld,gamecount=%d where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, 0, uid);
			}
		}
	}
	else
	{
		if (gameType != net::GAME_CATE_EVERYCOLOR && gameType != net::GAME_CATE_FISHING)
		{
			if (gameType == net::GAME_CATE_BULLFIGHT || gameType == net::GAME_CATE_BACCARAT || gameType == net::GAME_CATE_PAIJIU || gameType == net::GAME_CATE_DICE)// || gameType == net::GAME_CATE_WAR)
			{
				sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, uid);
			}
			else
			{
				if (isCoin)
				{
					sprintf(m_szSql, "UPDATE %s set winc=winc+%d,losec=losec+%d,daywinc=daywinc+%lld,maxwinc=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, uid);
				}
				else
				{
					sprintf(m_szSql, "UPDATE %s set win=win+%d,lose=lose+%d,daywin=daywin+%lld,maxwin=%lld where uid=%d;", gametables[gameType].c_str(), win, lose, winscore, maxscore, uid);
				}
			}
		}
	}
	if (bIsRobot == false)
	{
		LOG_DEBUG("geme_table_log - gameType:%d,uid:%d,winscore:%d,lExWinScore:%d,m_szSql:%s", gameType, uid, winscore, lExWinScore, m_szSql);

	}
	SendCommonLog(DB_INDEX_TYPE_ACC);
	//增加累计输赢
	
	int64 lAllScore = winscore + lExWinScore;
	uint64 uTime = getTime();

	//int rtmwelfare = 0;
	//if (rwelfare == 1 && lAllScore>0)
	//{
	//	rtmwelfare = 1;
	//}

	if (isCoin)
	{
		ZeroSqlBuff();
		sprintf(m_szSql, "UPDATE account%d SET win=win+%lld,welcount=%d,weltime=%lld where uid=%d;", CCommonLogic::GetDataTableNum(uid), lAllScore, welcount, uTime, uid);
		if (bIsRobot == false)
		{
			LOG_DEBUG("geme_account_log - gameType:%d,uid:%d,winscore:%d,lExWinScore:%d,m_szSql:%s", gameType, uid, winscore, lExWinScore, m_szSql);

		}

		SendCommonLog(DB_INDEX_TYPE_ACC);
	}

}

// 更新最大牌型
void    CDBMysqlMgr::UpdateGameMaxCard(uint16 gameType,uint32 uid,bool isCoin,uint8 cardData[],uint8 cardCount)
{
	LOG_DEBUG("gameType:%d,uid:%d", gameType, uid);
	if (gameType == net::GAME_CATE_EVERYCOLOR || gameType == net::GAME_CATE_FISHING)
	{
		return;
	}
    ZeroSqlBuff();
    Json::Value jvalue;
    for(uint8 i=0;i<cardCount;++i){
        jvalue.append(cardData[i]);
    }
    string logCard = jvalue.toFastString();
    if(isCoin){
        sprintf(m_szSql,"update %s set maxcardc='%s' where uid=%d;",gametables[gameType].c_str(),logCard.c_str(),uid);
    }else{
        sprintf(m_szSql,"update %s set maxcard='%s' where uid=%d;",gametables[gameType].c_str(),logCard.c_str(),uid);
    }
    SendCommonLog(DB_INDEX_TYPE_ACC);        
}    
// 重置每日盈利
void    CDBMysqlMgr::ResetGameDaywin(uint16 gameType,uint32 uid)
{
	LOG_DEBUG("gameType:%d,uid:%d", gameType, uid);
	if (gameType == net::GAME_CATE_EVERYCOLOR || gameType == net::GAME_CATE_FISHING)
	{
		return;
	}
    ZeroSqlBuff();
    sprintf(m_szSql,"update %s set daywin=0,daywinc=0 where uid=%d;",gametables[gameType].c_str(),uid);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}

void    CDBMysqlMgr::ResetGameWeekWin(uint16 gameType)
{
	//LOG_DEBUG("gameType:%d,uid:%d", gameType, uid);
	if (gameType != net::GAME_CATE_DICE)
	{
		return;
	}
	ZeroSqlBuff();
	//sprintf(m_szSql, "update %s set weekwinc=0;", gametables[gameType].c_str());
	//SendCommonLog(DB_INDEX_TYPE_ACC);
}

void    CDBMysqlMgr::ResetGameWeekWin(uint16 gameType, uint32 uid)
{
	//LOG_DEBUG("gameType:%d,uid:%d", gameType, uid);
	if (gameType != net::GAME_CATE_DICE)
	{
		return;
	}
	ZeroSqlBuff();
	//sprintf(m_szSql, "update %s set weekwinc=0 where uid=%d;", gametables[gameType].c_str(), uid);
	//SendCommonLog(DB_INDEX_TYPE_ACC);
}

void    CDBMysqlMgr::ChangeLandValue(uint32 uid,bool isCoin,int32 win,int32 lose,int32 land,int32 spring,int64 maxScore)
{
    ZeroSqlBuff();
    if(isCoin){
        sprintf(m_szSql, "UPDATE game_land set winc=winc+%d,losec=losec+%d,landc=landc+%d,springc=springc+%d,maxwinc=%lld where uid=%d;",win,lose,land,spring,maxScore,uid);
    }else{
        sprintf(m_szSql, "UPDATE game_land set win=win+%d,lose=lose+%d,land=land+%d,spring=spring+%d,maxwin=%lld where uid=%d;",win,lose,land,spring,maxScore,uid);
    }
    SendCommonLog(DB_INDEX_TYPE_ACC);
}

// 保存任务数据
void    CDBMysqlMgr::SaveUserMission(uint32 uid,map<uint32,stUserMission>& missions)
{
    ZeroSqlBuff();
    map<uint32,stUserMission>::iterator it = missions.begin();
	
    for(;it != missions.end();++it)
    {
		bool bIsUpdate = false;
		ZeroSqlBuff();
        stUserMission& refMiss = it->second;
        switch(refMiss.update)
        {
        case emDB_ACTION_UPDATE:
            {
                sprintf(m_szSql,"update umission%d set rtimes = %d,ctimes = %d,ptime = %d,cptime = %d where uid = %d and msid = %d",
                        uid % 100, refMiss.rtimes, refMiss.ctimes, refMiss.ptime, refMiss.cptime, uid,refMiss.msid);
				bIsUpdate = true;
                SendCommonLog(DB_INDEX_TYPE_MISSION);
            }break;
        case emDB_ACTION_INSERT:
            {
                sprintf(m_szSql,"insert into umission%d set rtimes = %d,ctimes = %d,ptime = %d,cptime = %d, uid = %d, msid = %d",
                        uid % 100, refMiss.rtimes, refMiss.ctimes, refMiss.ptime, refMiss.cptime, uid,refMiss.msid);
				bIsUpdate = true;
                SendCommonLog(DB_INDEX_TYPE_MISSION);
            }break;
        case emDB_ACTION_DELETE:
            {
                sprintf(m_szSql, "delete from umission%d where uid = %d and msid = %d", uid % 100, uid, refMiss.msid);
				bIsUpdate = true;
                SendCommonLog(DB_INDEX_TYPE_MISSION);
            }break;
        default:
            break;
        }
		if (bIsUpdate == true)
		{
			LOG_DEBUG("uid:%d,msid:%d,update:%d,sql:%s", uid, refMiss.msid, refMiss.update, m_szSql);

		}
    }
}
// 更新私人房收益
void    CDBMysqlMgr::ChangePrivateTableIncome(uint32 tableID,int64 hostIncome,int64 sysIncome)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE proom SET hostincome = hostincome + %lld,sysincome = sysincome + %lld where tableid=%d;",hostIncome,sysIncome,tableID);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 更新私人房过期时间
void    CDBMysqlMgr::UpdatePrivateTableDuetime(uint32 tableID,uint32 duetime)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"UPDATE proom SET duetime = %d where tableid=%d;",duetime,tableID);
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 插入充值订单记录
void    CDBMysqlMgr::InsertPayLog(uint32 uid,int64 rmb,int64 diamond)
{
    ZeroSqlBuff();
    sprintf(m_szSql,"insert into payfor set uid=%d,qty=%lld,rmb=%lld,ptime=%lld;",uid,diamond,rmb,getSysTime());
    SendCommonLog(DB_INDEX_TYPE_ACC);
}
// 发送邮件给玩家
void    CDBMysqlMgr::SendMail(uint32 sendID,uint32 recvID,string title,string content,string nickname)
{
    ZeroSqlBuff();
    //CGbkToUtf8 encode;
    //string title1,content1,nickname1;
    //encode.Convert(title,title1);
    //encode.Convert(content,content1);
    //encode.Convert(nickname,nickname1);
    
    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SEND_MAIL,emDBCALLBACK_AFFECT);
    pReq->params[0] = recvID;
    pReq->sqlStr  = CStringUtility::FormatToString("insert into emailuser set suid=%d,ruid=%d,title='%s',content='%s',nickname='%s',ptime=%lld;select @@identity;",sendID,recvID,title.c_str(),content.c_str(),nickname.c_str(),getSysTime());        
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq); 
}

// 保存游戏数据
void    CDBMysqlMgr::AsyncSaveSnatchCoinGameData(uint32 uid, uint32 type, string card)
{
	LOG_DEBUG("save - uid:%d,type:%d,card:%s", uid,type,card.c_str());

	string strtemp = card;

	size_t pos1 = strtemp.find("\r");
	if (pos1 != string::npos)
	{
		strtemp.erase(pos1);
	}
	size_t pos2 = strtemp.find("\n");
	if (pos2 != string::npos)
	{
		strtemp.erase(pos2);
	}
	LOG_DEBUG("save - uid:%d,type:%d,pos:%d - %d,size:%d %d,card:%s - %s", uid, type, pos1,pos2, card.size(), strtemp.size(), card.c_str(), strtemp.c_str());

	ZeroSqlBuff();

	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = uid;
	pReq->sqlStr = CStringUtility::FormatToString("insert into game_everycolor_snatchcoin set uid=%d,type=%d,card='%s';select @@identity;", uid,type, strtemp.c_str(), getSysTime());
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}

void    CDBMysqlMgr::AsyncSaveUserSnatchGameData(uint32 uid, uint32 snatch_type, uint32 player_type, string periods, string card)
{
	LOG_DEBUG("user - uid:%d,snatch_type:%d,player_type:%d,periods:%d,card:%s", uid, snatch_type, player_type, periods.c_str(), card.c_str());

	ZeroSqlBuff();

	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = uid;

	pReq->sqlStr = CStringUtility::FormatToString("insert into game_everycolor_usersnatch set uid=%d,snatchtype=%d,playertype=%d,card='%s' ON DUPLICATE KEY UPDATE card='%s';", uid, snatch_type, player_type, card.c_str(), card.c_str());
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}

// 清除夺宝数据
void    CDBMysqlMgr::ClearSnatchCoinGameData()
{

	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_CLEAR_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = 0;

	pReq->sqlStr = CStringUtility::FormatToString("delete from game_everycolor_snatchcoin;");
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);

}

void    CDBMysqlMgr::ClearUserSnatchGameData(uint32 snatch_type)
{
	
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_CLEAR_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = 0;

	pReq->sqlStr = CStringUtility::FormatToString("delete from game_everycolor_usersnatch where snatchtype=%d;", snatch_type);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);

}

// 调用通用sql
void    CDBMysqlMgr::SendCommonLog(uint8 dbType)
{
    string strsql(m_szSql);
    AddAsyncSql(dbType,strsql);
}
bool    CDBMysqlMgr::OnProcessDBEvent(CDBEventRep* pRep)
{
    if(m_pAsyncDBCallBack){
        m_pAsyncDBCallBack->OnProcessDBEvent(pRep);
    }  
    return true;
}

void    CDBMysqlMgr::AsyncLoadPlayerData(uint32 uid)
{
    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_LOAD_PLAYER_DATA,emDBCALLBACK_QUERY);
    pReq->params[0] = uid;
    pReq->sqlStr  = CStringUtility::FormatToString("SELECT * FROM user%d WHERE uid =%u limit 1;",CCommonLogic::GetDataTableNum(uid),uid);        
    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);    
}
void    CDBMysqlMgr::AsyncLoadAccountData(uint32 uid)
{
    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_LOAD_ACCOUNT_DATA,emDBCALLBACK_QUERY);
    pReq->params[0] = uid;
    pReq->sqlStr = CStringUtility::FormatToString(
		"SELECT a.*, IFNULL(b.recharge_actwle, 0) AS recharge_actwle, IFNULL(b.converts_actwle, 0) AS converts_actwle,passport.siteid as siteid FROM account%d a LEFT JOIN active_welfare b ON a.uid = b.uid left join passport on a.uid=passport.uid WHERE a.uid = %u LIMIT 1;",
		CCommonLogic::GetDataTableNum(uid), uid);
    //pReq->sqlStr  = CStringUtility::FormatToString("SELECT * FROM account%d WHERE uid =%u limit 1;",CCommonLogic::GetDataTableNum(uid),uid);        
    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq); 
}
void    CDBMysqlMgr::AsyncLoadMissionData(uint32 uid)
{
    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_MISSION]->MallocDBEventReq(emDBEVENT_LOAD_MISSION_DATA,emDBCALLBACK_QUERY);
    pReq->params[0] = uid;
    pReq->sqlStr  = CStringUtility::FormatToString("SELECT * FROM umission%d WHERE uid =%u;",uid%100,uid);
    m_pAsyncTask[DB_INDEX_TYPE_MISSION]->AsyncQuery(pReq);
}
// 加载游戏数据
void    CDBMysqlMgr::AsyncLoadGameData(uint32 uid,uint16 gameType)
{
	if (gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
	LOG_DEBUG("uid:%d gameType:%d", uid, gameType);
    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_LOAD_GAME_DATA,emDBCALLBACK_QUERY);
    pReq->params[0] = uid;
    pReq->params[1] = gameType;
    pReq->sqlStr  = CStringUtility::FormatToString("SELECT * FROM %s WHERE uid =%u limit 1;",gametables[gameType].c_str(),uid);
    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);         
}

//void    CDBMysqlMgr::AsyncLoadPayData(uint32 uid)
//{
//	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_OTHER]->MallocDBEventReq(emDBEVENT_LOAD_PAY_DATA, emDBCALLBACK_QUERY);
//	pReq->params[0] = uid;
//	pReq->sqlStr = CStringUtility::FormatToString("SELECT * FROM pay WHERE uid =%u;", uid);
//	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
//	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
//}

// 加载登陆机器人
bool    CDBMysqlMgr::AsyncLoadRobotLogin(uint16 gameType, uint8 level, uint8 day,uint32 robotNum, uint32 batchID, uint32 loginType, uint32 leveltype)
{
	bool bRetMysql = true;
    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_LOAD_ROBOT_LOGIN,emDBCALLBACK_QUERY);
    pReq->params[0] = robotNum;
	pReq->params[1] = level;
	pReq->params[2] = gameType;
	pReq->params[3] = batchID;
	pReq->params[4] = leveltype;
	if (leveltype == net::ROOM_CONSUME_TYPE_COIN)
	{
		pReq->sqlStr = CStringUtility::FormatToString("select * from robot where d%d = 1 and status=0 and gametype=%d and loginstate=0 and richlv=%d and logintype=0 ORDER BY logincount limit %d;", day, gameType, level, robotNum);
	}
	else if (leveltype == net::ROOM_CONSUME_TYPE_SCORE)
	{
		pReq->sqlStr = CStringUtility::FormatToString("select * from robot where d%d = 1 and status=0 and gametype=%d and loginstate=0 and scorelv=%d and logintype=0 ORDER BY logincount limit %d;", day, gameType, level, robotNum);
	}
	else
	{
		bRetMysql = false;
		LOG_ERROR("error_type gameType:%d,level:%d,day:%d,robotNum:%d,batchID:%d,loginType:%d,leveltype:%d",
			gameType, level, day, robotNum, batchID, loginType, leveltype);
	}
	LOG_DEBUG("bRetMysql:%d,gameType:%d,batchID:%d,leveltype:%d,sql:%s", bRetMysql, gameType, batchID, leveltype, pReq->sqlStr.c_str());
    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
	return bRetMysql;
}

void    CDBMysqlMgr::AsyncLoadTimeIntervalRobotLogin(uint16 gameType,uint32 roomid, uint8 level, uint8 day, uint32 robotNum, uint32 batchID, uint32 loginType)
{
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_LOAD_TIME_ROBOT_LOGIN, emDBCALLBACK_QUERY);
	pReq->params[0] = robotNum;
	pReq->params[1] = level;
	pReq->params[2] = batchID;
	pReq->sqlStr = CStringUtility::FormatToString("select * from robot where d%d = 1 and status=0 and gametype=%d and roomid=%d and loginstate=0 and richlv=%d and logintype=1 ORDER BY logincount limit %d;", day, gameType, roomid, level, robotNum);

	LOG_DEBUG("gameType:%d,batchID:%d,sql:%s", gameType,batchID, pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}


void    CDBMysqlMgr::AsyncSaveUserBankruptScore(uint32 uid, uint16 gameType, uint16 roomID, int64 oldValue,int64 newValue, int64 enter_min, uint64 utime)
{
	//LOG_DEBUG("user - uid:%d,gameType:%d,roomID:%d,oldValue:%lld,newValue:%lld,enter_min:%lld,utime:%lld",
	//	uid, gameType, roomID, oldValue, newValue, enter_min, utime);

	ZeroSqlBuff();

	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = uid;

	pReq->sqlStr = CStringUtility::FormatToString("insert into bankrupt set uid=%d,gameid=%d,roomid=%d,beforeCoin=%lld,afterCoin=%lld,limitCoin=%lld,time=%lld;", uid, gameType, roomID, oldValue, newValue, enter_min, utime);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}


// 修改用户库存数值（增量修改）
void    CDBMysqlMgr::ChangeStockScore(uint16 gametype,uint32 uid, int64 score)
{
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->sqlStr = CStringUtility::FormatToString("UPDATE %s SET stockscore=stockscore+%d WHERE uid=%d;", gametables[gametype].c_str(), score, uid);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}

void  CDBMysqlMgr::UpdateGameRoomParam(string param,uint16 gametype, uint16 roomid)
{
	LOG_DEBUG("save - gametype:%d,roomid:%d,param:%s", gametype, roomid, param.c_str());

	string strtemp = param;

	size_t pos1 = strtemp.find("\r");
	if (pos1 != string::npos)
	{
		strtemp.erase(pos1);
	}
	size_t pos2 = strtemp.find("\n");
	if (pos2 != string::npos)
	{
		strtemp.erase(pos2);
	}

	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_CFG]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->sqlStr = CStringUtility::FormatToString("UPDATE roomcfg SET param='%s' WHERE gametype =%d and roomid=%d;", strtemp.c_str(), gametype, roomid);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_CFG]->AsyncQuery(pReq);
}

void    CDBMysqlMgr::AsyncInsertBairenCount(struct tagBairenCount & BairenCount)
{
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = BairenCount.uid;
	pReq->sqlStr = CStringUtility::FormatToString("insert into game_bairen_count set chessid='%s',svrid=%d,gametype=%d,roomid=%d,tableid=%d,uid=%d,winscore=%lld,betscore=%lld;", BairenCount.chessid.c_str(), m_svrID, BairenCount.gametype, BairenCount.roomid, BairenCount.tableid, BairenCount.uid, BairenCount.winscore, BairenCount.betscore);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}


void    CDBMysqlMgr::AsyncRemoveBairenCount(struct tagBairenCount & BairenCount)
{
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_ACC]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_AFFECT);
	pReq->params[0] = BairenCount.uid;
	
	pReq->sqlStr = CStringUtility::FormatToString("delete from game_bairen_count where chessid='%s';", BairenCount.chessid.c_str());
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq);
}

void  CDBMysqlMgr::ClearBairenCount()
{
	ZeroSqlBuff();
	sprintf(m_szSql, "delete from game_bairen_count where svrid=%d;", m_svrID);
	SendCommonLog(DB_INDEX_TYPE_ACC);
}

void    CDBMysqlMgr::SaveControlPlayerData(struct tagControlPalyer & ControlPalyer)
{
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_CFG]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_NULL);
	pReq->sqlStr = CStringUtility::FormatToString("UPDATE control_user_coin_log SET end_time=%lld,before_coin=%lld,later_coin=%lld,gamecount=%d WHERE id=%d;",
		ControlPalyer.etime, ControlPalyer.bscore, ControlPalyer.escore, ControlPalyer.rccount, ControlPalyer.id);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_CFG]->AsyncQuery(pReq);
}

void    CDBMysqlMgr::SaveControlPlayerData(struct tagControlMultiPalyer & ControlPalyer)
{
	CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_CFG]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA, emDBCALLBACK_NULL);
	pReq->sqlStr = CStringUtility::FormatToString("UPDATE control_user_coin_log SET end_time=%lld,before_coin=%lld,later_coin=%lld,gamecount=%d WHERE id=%d;",
		ControlPalyer.etime, ControlPalyer.bscore, ControlPalyer.escore, ControlPalyer.rccount, ControlPalyer.id);
	LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
	m_pAsyncTask[DB_INDEX_TYPE_CFG]->AsyncQuery(pReq);
}
// 添加自动杀分数据记录
//void    CDBMysqlMgr::AddAutoKillData(uint32 uid,uint32 gid,uint32 rid,int64 newSrcoe, int64 oldSrcoe)
//{
//    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_CFG]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA,emDBCALLBACK_AFFECT);
//    pReq->sqlStr  = CStringUtility::FormatToString("insert into control_user_coin_log set uid=%d,gid=%d,rid=%d,before_coin=%lld,later_coin=%lld,type=2,control_type=2,start_time=%lld;",uid,gid,rid,oldSrcoe,newSrcoe,getSysTime());        
//    LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
//    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq); 
//}
// 更新自动杀分玩家数据
//void    CDBMysqlMgr::SaveAutoKillPlayerData(uint32 uid,int64 coin)
//{
//    CDBEventReq *pReq = m_pAsyncTask[DB_INDEX_TYPE_CFG]->MallocDBEventReq(emDBEVENT_SAVE_GAME_DATA,emDBCALLBACK_NULL);
//    pReq->sqlStr  = CStringUtility::FormatToString("UPDATE automatic_killing SET gamecount=gamecount+1,coin=coin+%lld,utime=%lld WHERE uid=%d;",coin,getSysTime(),uid);        
//    LOG_DEBUG("sql:%s", pReq->sqlStr.c_str());
//    m_pAsyncTask[DB_INDEX_TYPE_ACC]->AsyncQuery(pReq); 
//}

// 插入活跃福利的玩家充值与提现数据
void    CDBMysqlMgr::InsertActiveWelfare(uint32 uid, int64 recharge_actwle, int64 converts_actwle)
{
    LOG_DEBUG("Insert Into ActiveWelfare uid:%d recharge_actwle:%d converts_actwle:%d", uid, recharge_actwle, converts_actwle);
    ZeroSqlBuff();
    sprintf(m_szSql, "insert into active_welfare values(%d,%lld,%lld,%lld);", uid, recharge_actwle, converts_actwle, getTime());
    SendCommonLog(DB_INDEX_TYPE_ACC);
}

// 记录玩家在活跃福利的数据
void    CDBMysqlMgr::InsertActiveWelfareLog(uint32 uid, uint8 game_id, int64 base_loss, string loss_range, uint64 welfare, uint8 status, int64 current_loss, int64 win_coin, uint64 sys_time)
{   
    ZeroSqlBuff();
    sprintf(m_szSql, "insert into active_welfare_log values(0,%d,%d,%lld,%s,%lld,%d,%lld,%lld,%llu);", uid, game_id, base_loss, loss_range.c_str(), welfare, status, current_loss, win_coin, sys_time);
    LOG_DEBUG("Insert Into InsertActiveWelfareLog m_szSql:%s", m_szSql);
    SendCommonLog(DB_INDEX_TYPE_LOG);
}

// 记录新注册玩家福利的数据
void    CDBMysqlMgr::InsertNewRegisterWelfareLog(uint32 uid, uint8 game_id, uint32 curr_must_win, uint32 curr_total_win, uint64 curr_win_coin, uint64 total_win_coin, uint32 status, uint64 sys_time)
{
	ZeroSqlBuff();
	sprintf(m_szSql, "insert into new_register_welfare_log values(0,%d,%d,%d,%d,%llu,%llu,%d,%llu);", uid, game_id, curr_must_win, curr_total_win, curr_win_coin, total_win_coin, status, sys_time);
	LOG_DEBUG("Insert Into InsertNewRegisterWelfareLog m_szSql:%s", m_szSql);
	SendCommonLog(DB_INDEX_TYPE_LOG);
}

// 更新库存信息 add by har
void CDBMysqlMgr::UpdateStock(uint16 gameType, uint16 roomId, int64 addStockScore, int64 addJackpot) {
	ZeroSqlBuff();
	sprintf(m_szSql, "update room_stock_cfg set stock=stock+%lld,jackpot=jackpot+%lld where gametype=%d and roomid=%d;",
		addStockScore, addJackpot, gameType, roomId);
	LOG_DEBUG("UpdateStock sql:%s", m_szSql);
	SendCommonLog(DB_INDEX_TYPE_CFG);
}

void    CDBMysqlMgr::ChangeGameValueForFish(uint32 uid, bool isCoin, int64 winscore, int welcount)
{
	//更新游戏记录
	if (isCoin)
	{
		ZeroSqlBuff();
		sprintf(m_szSql, "UPDATE %s set maxwinc=maxwinc+%lld where uid=%d;", gametables[net::GAME_CATE_FISHING].c_str(), winscore, uid);
		LOG_DEBUG("update game_fishing sql:%s", m_szSql);
		SendCommonLog(DB_INDEX_TYPE_ACC);
	}		
		
	//更新用户表相关数据
	if (isCoin)
	{
		ZeroSqlBuff();
		uint64 uTime = getTime();
		sprintf(m_szSql, "UPDATE account%d SET win=win+%lld,welcount=%d,weltime=%lld where uid=%d;", CCommonLogic::GetDataTableNum(uid), winscore, welcount, uTime, uid);
		SendCommonLog(DB_INDEX_TYPE_ACC);
		LOG_DEBUG("update account0 sql:%s", m_szSql);
	}
}