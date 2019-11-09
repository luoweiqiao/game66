
#include "async_dbcallback.h"
#include "svrlib.h"
#include "stdafx.h"
#include "player.h"
#include "json/json.h"
#include "gobal_robot_mgr.h"

using namespace svrlib;

bool CLobbyAsyncDBCallBack::OnProcessDBEvent(CDBEventRep* pRep)
{
    switch(pRep->eventID)
    {
    case emDBEVENT_LOAD_PLAYER_DATA:
        {
            OnLoadPlayerDataEvent(pRep);
        }break;
    case emDBEVENT_LOAD_ACCOUNT_DATA:
        {
            OnLoadAccountDataEvent(pRep);
        }break;
    case emDBEVENT_LOAD_GAME_DATA:
        {
            OnLoadGameDataEvent(pRep);
        }break;
    case emDBEVENT_LOAD_MISSION_DATA:
        {
            OnLoadMissionDataEvent(pRep);
        }break;
	//case emDBEVENT_LOAD_PAY_DATA:
	//	{
	//		OnLoadPayDataEvent(pRep);
	//	}break;
    case emDBEVENT_LOAD_ROBOT_LOGIN:
        {
            OnLoadRobotLoginDataEvent(pRep);
        }break;
	case emDBEVENT_LOAD_TIME_ROBOT_LOGIN:
	{
		OnLoadRobotTimeLoginDataEvent(pRep);
	}break;
    case emDBEVENT_SEND_MAIL:
        {
            OnSendMailEvent(pRep);
        }break;
    default:
        break;
    }
	//COUNT_OCCUPY_MEM_SZIE(this);
    return true;
}
void CLobbyAsyncDBCallBack::OnLoadPlayerDataEvent(CDBEventRep* pRep)
{
    //LOG_DEBUG("OnLoadPlayerData");
    stPlayerBaseInfo data;
    if(pRep->vecData.size() > 0)
    {
        map<string, MYSQLValue>& refRows = pRep->vecData[0];
        uint32 uid          = refRows["uid"].as<uint32>();
        data.name           = refRows["nickname"].as<string>();
        data.sex            = refRows["sex"].as<uint32>();
		data.rtime		    = refRows["rtime"].as<uint32>();
        data.offlinetime    = refRows["offlinetime"].as<uint32>();
        data.safepwd        = refRows["safepwd"].as<string>();
        data.clogin         = refRows["clogin"].as<uint32>();
        data.weekLogin      = refRows["weeklogin"].as<uint32>();
        data.reward         = refRows["reward"].as<uint32>();
        data.bankrupt       = refRows["bankrupt"].as<uint32>();
        uint32 headType     = refRows["imagetype"].as<uint32>();
        if(headType == 1){
            data.headIcon   = refRows["imageurl"].as<uint32>();
        }
        data.dayGameCount   = refRows["dgcount"].as<uint32>();
		string deviceID	    = refRows["deviceid"].as<string>();
        string ldeviceID    = refRows["ldeviceid"].as<string>();
		string rip = refRows["ip"].as<string>();
		string str_loginip	= refRows["loginip"].as<string>();
		data.level			= refRows["level"].as<int32>();
		data.ispay          = refRows["ispay"].as<uint32>();

        CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
        if(pPlayer != NULL && pPlayer->GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        {
            pPlayer->SetBaseInfo(data);
			pPlayer->SetLoginDeviceID(ldeviceID);
			pPlayer->SetIP(str_loginip);
            if(pPlayer->IsLoadOver()){
                pPlayer->OnGetAllData();
            }
        }else{
            LOG_DEBUG("the player is not find:%d--%s",uid,data.name.c_str());
        }
    }else{
        LOG_ERROR("the base data is can't load:%d, vecData.size:%d",pRep->params[0], pRep->vecData.size());
        uint32 uid = pRep->params[0];
        if(uid >= ROBOT_MGR_ID){
            CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
            if(pPlayer != NULL && pPlayer->GetPlayerState() == PLAYER_STATE_LOAD_DATA)
            {
                pPlayer->SetBaseInfo(data);
                if(pPlayer->IsLoadOver()){
                    pPlayer->OnGetAllData();
                }
            }
        }
    }    
}
void CLobbyAsyncDBCallBack::OnLoadAccountDataEvent(CDBEventRep* pRep)
{
    //LOG_DEBUG("OnLoadAccountData");
    stAccountInfo data;
    if(pRep->vecData.size() > 0)
    {
        map<string, MYSQLValue>& refRows = pRep->vecData[0];
        uint32 uid = refRows["uid"].as<uint32>();
        data.diamond = refRows["diamond"].as<int64>();
        data.coin    = refRows["coin"].as<int64>();
        data.ingot   = refRows["ingot"].as<int64>();
        data.score   = refRows["score"].as<int64>();
        data.cvalue  = refRows["cvalue"].as<int64>();
        data.vip     = refRows["vip"].as<uint32>();
        data.safecoin = refRows["safecoin"].as<int64>();
		data.recharge = refRows["Recharge"].as<int64>();
		data.converts = refRows["converts"].as<int64>();
		data.userright = refRows["userright"].as<uint32>();
		data.win = refRows["win"].as<int64>();
		data.posrmb = refRows["posrmb"].as<uint32>();
		data.postime = refRows["postime"].as<uint32>();
		data.welcount = refRows["welcount"].as<int32>();
		data.weltime = refRows["weltime"].as<uint64>();
        data.recharge_actwle = refRows["recharge_actwle"].as<int64>();
        data.converts_actwle = refRows["converts_actwle"].as<int64>();
		data.siteid = refRows["siteid"].as<int32>();

        CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
        if(pPlayer != NULL && pPlayer->GetPlayerState() == PLAYER_STATE_LOAD_DATA )
        {
            //如果活跃福利的值获取为空，则需要在active_welfare表中插入相关值，不包括机器人
            if( pPlayer->IsRobot() == false)
            {            
                if (data.recharge_actwle <= 0)
                {
                    data.recharge_actwle = data.coin + data.safecoin;
                    CDBMysqlMgr::Instance().InsertActiveWelfare(uid, data.recharge_actwle, 0);
                    LOG_DEBUG("player login insert active_welfare table. uid:%d recharge_actwle:%d converts_actwle:%d", uid, data.recharge_actwle, 0);
                }
                else
                {
                    LOG_DEBUG("player login is exist active_welfare table. uid:%d recharge_actwle:%d converts_actwle:%d", uid, data.recharge_actwle, data.converts_actwle);
                }
            }
            else
            {
                LOG_DEBUG("the player is robot uid:%d", uid);
            }
            pPlayer->SetAccountInfo(data);
            if(pPlayer->IsLoadOver())
            {
                pPlayer->OnGetAllData();
            }                  
        }
        else
        {
            LOG_DEBUG("the load account data player is not find:%d",uid);
        }
    }else{
        LOG_ERROR("account data is can't load:%d",pRep->params[0]);
    }
}

void CLobbyAsyncDBCallBack::OnLoadMissionDataEvent(CDBEventRep *pRep)
{
    //LOG_DEBUG("OnLoadMissionData");
    uint32 uid = pRep->params[0];
    CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));

	int status = -1;
	if (pPlayer!=NULL)
	{
		status = pPlayer->GetPlayerState();
	}

	LOG_DEBUG("uid:%d,pPlayer:%p,status:%d,pdatasize:%d", uid, pPlayer, status, pRep->vecData.size());

    if(pPlayer == NULL || pPlayer->GetPlayerState() != PLAYER_STATE_LOAD_DATA)
    {
        LOG_DEBUG("load mission player is not find:%d",uid);
        return;
    }
    map<uint32,stUserMission> mission;
    if(pRep->vecData.size() > 0)
    {
        for(uint32 i = 0;i < pRep->vecData.size();++i){
            map<string, MYSQLValue>& refRows = pRep->vecData[i];
            stUserMission data;
            data.ctimes = refRows["ctimes"].as<uint32>();
            data.msid = refRows["msid"].as<uint32>();
            data.ptime = refRows["ptime"].as<uint32>();
            data.rtimes = refRows["rtimes"].as<uint32>();
            data.cptime = refRows["cptime"].as<uint32>();
            data.update = 0;
            mission[data.msid] = data;
        }
    }
    pPlayer->GetMissionMgr().SetMission(mission);
    if(pPlayer->IsLoadOver()){
        pPlayer->OnGetAllData();
    }
}
void    CLobbyAsyncDBCallBack::OnLoadGameDataEvent(CDBEventRep* pRep)
{
    uint16 gameType = pRep->params[1];
    LOG_DEBUG("Load game data callback gameType:%d vecData size:%d", gameType, pRep->vecData.size());
    stGameBaseInfo data;    
    if(pRep->vecData.size() > 0)
    {        
        map<string, MYSQLValue>& refRows = pRep->vecData[0];
        uint32 uid           = refRows["uid"].as<uint32>();               
        data.win             = refRows["win"].as<uint32>();
        data.lose            = refRows["lose"].as<uint32>();
        data.maxwin          = refRows["maxwin"].as<int64>();
        data.winc            = refRows["winc"].as<uint32>();
        data.losec           = refRows["losec"].as<uint32>();
        data.maxwinc         = refRows["maxwinc"].as<int64>();
        data.daywin          = refRows["daywin"].as<int64>();
        data.daywinc         = refRows["daywinc"].as<int64>();

		//if (gameType == net::GAME_CATE_DICE)
		//{
		//	data.weekwinc = refRows["weekwinc"].as<int64>();
		//}

		if (gameType == net::GAME_CATE_TEXAS || gameType == net::GAME_CATE_SHOWHAND)
		{
			data.totalwinc  = refRows["totalwinc"].as<int64>();
		}
		if (gameType == net::GAME_CATE_FRUIT_MACHINE)
		{
			data.stockscore = refRows["stockscore"].as<int64>();
			data.gamecount = refRows["gamecount"].as<int64>();
		}
        if(gameType == net::GAME_CATE_LAND)
		{
            data.land        = refRows["land"].as<uint32>();
            data.spring      = refRows["spring"].as<uint32>();
            data.landc       = refRows["landc"].as<uint32>();
            data.springc     = refRows["springc"].as<uint32>();
        }
		else
		{
            // 解析牌型
            string strCard  = refRows["maxcard"].as<string>();
            string strCardc = refRows["maxcardc"].as<string>();
            Json::Reader reader;
            Json::Value  jvalue;
            if(reader.parse(strCard,jvalue))
			{
                for(uint8 i=0;i<jvalue.size() && i<5;++i)
				{
                    data.bestCard[i] = jvalue[i].asUInt();
                }
            }
            if(reader.parse(strCardc,jvalue))
			{
                for(uint8 i=0;i<jvalue.size() && i<5;++i)
				{
                    data.bestCardc[i] = jvalue[i].asUInt();
                }
            }
        }
		
		// ----------------------------------------------------------------------------------------

        CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
        if(pPlayer != NULL && pPlayer->GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        {
            pPlayer->SetGameInfo(gameType,data);
            if(pPlayer->IsLoadOver())
			{
                pPlayer->OnGetAllData();
            }
        }
		else
		{
            LOG_DEBUG("load gamedata player not find:%d",uid);
        } 
    }
	else
	{
        LOG_DEBUG("gamedata not load,insert data - uid:%d",pRep->params[0]);
        uint32 uid = pRep->params[0];
        CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
        if(pPlayer != NULL && pPlayer->GetPlayerState() == PLAYER_STATE_LOAD_DATA)
        {
            pPlayer->SetGameInfo(gameType,data);
            if(pPlayer->IsLoadOver())
			{
                pPlayer->OnGetAllData();
            }
            CDBMysqlMgr::Instance().InsertGameValue(gameType,uid);
        }
    }
}

//void    CLobbyAsyncDBCallBack::OnLoadPayDataEvent(CDBEventRep* pRep)
//{
//	int64 uid = 0;
//	uint32 vecDataSize = 0;
//	if (pRep != NULL)
//	{
//		uid = pRep->params[0];
//		vecDataSize = pRep->vecData.size();
//	}
//	CPlayer* pPlayer = dynamic_cast<CPlayer*>(CPlayerMgr::Instance().GetPlayer(uid));
//	LOG_DEBUG("pay_data - uid:%lld,vecDataSize:%d,pPlayer:%p", uid, vecDataSize, pPlayer);
//	vector<stPlayerPayData> vecPlayerPayData;
//	for (uint32 i = 0; i < pRep->vecData.size(); ++i)
//	{
//		map<string, MYSQLValue>& refRows = pRep->vecData[i];
//		stPlayerPayData data;
//		data.uid = refRows["uid"].as<uint32>();
//		data.pid = refRows["pid"].as<uint32>();
//		data.rmb = refRows["rmb"].as<uint32>();
//		data.coin = refRows["coin"].as<uint32>();
//		LOG_DEBUG("uid:%d,pid:%d,rmb:%d,coin:%d", data.uid, data.pid, data.rmb, data.coin);
//		vecPlayerPayData.push_back(data);
//	}
//	if (pPlayer != NULL)
//	{
//		pPlayer->SetPlayerPayData(vecPlayerPayData);
//	}
//}

void    CLobbyAsyncDBCallBack::OnLoadRobotLoginDataEvent(CDBEventRep* pRep)
{
	int64 robotNum = 0;
	int64 level = 0;
	int64 gameType = 0;
	int64 batchID = 0;
	int64 leveltype = 0;
	uint32 vecDataSize = 0;
	if (pRep != NULL)
	{
		robotNum = pRep->params[0];
		level = pRep->params[1];
		gameType = pRep->params[2];
		batchID = pRep->params[3];
		leveltype = pRep->params[4];
		vecDataSize = pRep->vecData.size();
	}
	LOG_DEBUG("load_robot_count - robotNum:%lld,level:%lld,gameType:%lld,batchID:%lld,leveltype:%lld,vecDataSize:%d", robotNum, level, gameType, batchID,leveltype, vecDataSize);
    //map<uint32,stUserMission> mission;
	CGobalRobotMgr::Instance().UpdateLoadRobotStatus(gameType, false);

    map<uint32,stRobotCfg> mapRobotCfg;
    for(uint32 i = 0;i < pRep->vecData.size();++i)
	{
        map<string, MYSQLValue>& refRows = pRep->vecData[i];
        stRobotCfg cfg;
		cfg.batchID			= batchID;
		cfg.leveltype		= leveltype;
        cfg.uid             = refRows["uid"].as<uint32>();
        cfg.actionType      = refRows["attype"].as<uint32>();
        cfg.playerType      = refRows["pltype"].as<uint32>();
        cfg.scoreLv         = refRows["scorelv"].as<uint32>();
        cfg.richLv          = refRows["richlv"].as<uint32>();
        cfg.status          = refRows["status"].as<uint32>();
        cfg.actiontime      = refRows["actiontime"].as<uint32>();
        cfg.gameType        = refRows["gametype"].as<uint32>();
		cfg.loginType       = refRows["logintype"].as<uint32>();
		uint32 logincount = refRows["logincount"].as<uint32>();
		LOG_DEBUG("batchID:%d, leveltype:%d, uid:%d, scoreLv:%d, richLv:%d, gameType:%d, logincount:%d", cfg.batchID, cfg.leveltype, cfg.uid, cfg.scoreLv, cfg.richLv, cfg.gameType, logincount);

        mapRobotCfg.insert(make_pair(cfg.uid,cfg));
    }
	bool bLoadRobotError = false;
	int iErrorCount = 0;
	map<uint32, stRobotCfg>::iterator it = mapRobotCfg.begin();
    for(;it != mapRobotCfg.end();++it)
    {
        stRobotCfg & refCfg = it->second;
		if (CPlayerMgr::Instance().IsOnline(refCfg.uid))
		{
			bLoadRobotError = true;
			iErrorCount++;
			continue;
		}
        CGobalRobotMgr::Instance().LoginRobot(refCfg);
    }

	LOG_DEBUG("load_robot_end - robotNum:%lld,level:%lld,gameType:%lld,batchID:%lld,leveltype:%lld,vecDataSize:%d,bLoadRobotError:%d,iErrorCount:%d",
		robotNum, level, gameType, batchID, leveltype, vecDataSize, bLoadRobotError, iErrorCount);

}

void    CLobbyAsyncDBCallBack::OnLoadRobotTimeLoginDataEvent(CDBEventRep* pRep)
{
	int64 robotNum = 0;
	int64 level = 0;
	int64 batchID = 0;
	uint32 vecDataSize = 0;
	if (pRep != NULL)
	{
		robotNum = pRep->params[0];
		level = pRep->params[1];
		batchID = pRep->params[2];
		vecDataSize = pRep->vecData.size();
	}
	LOG_DEBUG("load_robot_count - robotNum:%lld,level:%lld,batchID:%lld,vecDataSize:%d", robotNum, level, batchID, vecDataSize);
	//map<uint32,stUserMission> mission;
	map<uint32, stRobotCfg> mapRobotCfg;
	for (uint32 i = 0; i < pRep->vecData.size(); ++i) {
		map<string, MYSQLValue>& refRows = pRep->vecData[i];
		stRobotCfg cfg;
		cfg.batchID = batchID;
		cfg.uid = refRows["uid"].as<uint32>();
		cfg.actionType = refRows["attype"].as<uint32>();
		cfg.playerType = refRows["pltype"].as<uint32>();
		cfg.scoreLv = refRows["scorelv"].as<uint32>();
		cfg.richLv = refRows["richlv"].as<uint32>();
		cfg.status = refRows["status"].as<uint32>();
		cfg.actiontime = refRows["actiontime"].as<uint32>();
		cfg.gameType = refRows["gametype"].as<uint32>();
		cfg.loginType = refRows["logintype"].as<uint32>();
		uint32 logincount = refRows["logincount"].as<uint32>();
		LOG_DEBUG("batchID:%d,uid:%d, scoreLv:%d, richLv:%d, gameType:%d, logincount:%d", cfg.batchID, cfg.uid, cfg.scoreLv, cfg.richLv, cfg.gameType, logincount);

		mapRobotCfg.insert(make_pair(cfg.uid, cfg));
	}
	bool bLoadRobotError = false;
	int iErrorCount = 0;
	map<uint32, stRobotCfg>::iterator it = mapRobotCfg.begin();
	for (; it != mapRobotCfg.end(); ++it)
	{
		stRobotCfg & refCfg = it->second;
		if (CPlayerMgr::Instance().IsOnline(refCfg.uid))
		{
			bLoadRobotError = true;
			iErrorCount++;
			continue;
		}
		CGobalRobotMgr::Instance().LoginTimeRobot(refCfg);
	}
	LOG_DEBUG("load_robot_end - robotNum:%lld,level:%lld,batchID:%lld,vecDataSize:%d,bLoadRobotError:%d,iErrorCount:%d", robotNum, level, batchID, vecDataSize, bLoadRobotError,iErrorCount);

}


void    CLobbyAsyncDBCallBack::OnSendMailEvent(CDBEventRep* pRep)
{
    uint32 uid = pRep->params[0];               
    Json::Value  jvalue;    
    jvalue["action"] = "getSigninEmail";

    string broadCast = jvalue.toFastString();
    LOG_DEBUG("OnSendMail Event:%d--%s",uid,broadCast.c_str());
    
    net::msg_php_broadcast_rep rep;
    rep.set_msg(broadCast);
    
    CPlayerMgr::Instance().SendMsgToPlayer(&rep,net::S2C_MSG_PHP_BROADCAST,uid);        
}




















