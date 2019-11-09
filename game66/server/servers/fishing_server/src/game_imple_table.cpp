//
// Created by Joe on 19/6/3.
//
#include <center_log.h>
#include "game_imple_table.h"
#include "stdafx.h"
#include "game_room.h"
#include "json/json.h"
#include "fish_info_mgr.h"

using namespace std;
using namespace svrlib;
using namespace net;

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 捕鱼普通场
        {
            pTable = new CGameFishingTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛捕鱼
        {
            pTable = new CGameFishingTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房捕鱼
        {
            pTable = new CGameFishingTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}

// 捕鱼游戏桌子
CGameFishingTable::CGameFishingTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

	//设置开始定时器
	SetGameState(net::TABLE_STATE_FISHING_PAUSE);
	const static uint32 s_FreeTime = 5 * 1000;       // 空闲时间
	m_coolLogic.beginCooling(s_FreeTime + GetTableID()*1000);
	m_map_level = 0;
	m_map_cycle_count = 0;
	ZeroMemory(m_lTableScore, sizeof(m_lTableScore));
	ZeroMemory(m_lTableBulletID, sizeof(m_lTableBulletID));
	m_map_list.clear();
	m_map_pos = 0;
	m_bot_list.clear();
	m_create_fish_end = false;
	m_curr_map_info.clear();
	m_bullet_classification.clear();
	m_kill_classification.clear();
	m_player_novicectrlinfo.clear();
}

CGameFishingTable::~CGameFishingTable()
{
        
}

bool    CGameFishingTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer->GetTable() != NULL)
	{
		return false;
	}
        
    // 限额进入
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin()){
        return false;
    }	
    return true;
}

bool    CGameFishingTable::CanLeaveTable(CGamePlayer* pPlayer)
{   
    return true;
}

bool CGameFishingTable::LeaveTable(CGamePlayer* pPlayer, bool bNotify) {
	bool bRet = CGameTable::LeaveTable(pPlayer, bNotify);
	if (bRet) 
	{
		int uPlayerCount = GetPlayerNum();		
	}
	return bRet;
}

void CGameFishingTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::fishing_table_info* pfishing = pInfo->mutable_fishing();
	pfishing->set_tableid(GetTableID());
	pfishing->set_tablename(m_conf.tableName);
    pfishing->set_basescore(m_conf.baseScore);
	pfishing->set_consume(m_conf.consume);
	pfishing->set_entermin(m_conf.enterMin);	
	pfishing->set_maplevel(m_map_level);
}

//配置桌子
bool CGameFishingTable::Init()
{
    SetGameState(net::TABLE_STATE_FISHING_PAUSE);
    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    string param = m_pHostRoom->GetCfgParam();    
    Json::Reader reader;
    Json::Value  jvalue;
    if(!reader.parse(param,jvalue))
    {
        LOG_ERROR("解析游戏参数json错误:%s",param.c_str());
        return true;
    }	

	ReAnalysisParam();
    return true;
}

bool CGameFishingTable::ReAnalysisParam()
{
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}	

	//解析map_level配置
	m_map_list.clear();
	string map_list = "";
	if (jvalue.isMember("map_level"))
	{
		map_list = jvalue["map_level"].asString();
		//解析地图ID
		if (map_list.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(map_list, jvalue))
			{
				LOG_ERROR("解析 map_list:%s json串错误", map_list.c_str());
				return false;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				m_map_list.push_back(jvalue[i].asUInt());
				LOG_DEBUG("add map id:%d", jvalue[i].asUInt());
			}
		}
	}
	
	//解析bot_list配置
	m_bot_list.clear();
	string bot_list = "";
	if (jvalue.isMember("bot_list"))
	{
		bot_list = jvalue["bot_list"].asString();
		//解析底分列表
		if (bot_list.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(bot_list, jvalue))
			{
				LOG_ERROR("解析 bot_list:%s json串错误", bot_list.c_str());
				return false;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				m_bot_list.push_back(jvalue[i].asUInt());
				LOG_DEBUG("add bot:%d", jvalue[i].asUInt());
			}
		}
	}

	//解析aim_range配置
	m_arm_range_list.clear();
	string arm_list = "";
	if (jvalue.isMember("aim_range"))
	{
		arm_list = jvalue["aim_range"].asString();
		//解析瞄准倍数范围
		if (arm_list.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(arm_list, jvalue))
			{
				LOG_ERROR("解析 arm_list:%s json串错误", arm_list.c_str());
				return false;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				m_arm_range_list.push_back(jvalue[i].asUInt());
				LOG_DEBUG("add arm range:%d", jvalue[i].asUInt());
			}
		}
	}

	//解析pro_elevation配置---提升/降低击中鱼的概率
	vector<uint32> vec_pro_elev;
	vec_pro_elev.clear();
	string pro_elev_list = "";
	if (jvalue.isMember("pro_elev_list"))
	{
		pro_elev_list = jvalue["pro_elev_list"].asString();

		//解析概率列表
		if (pro_elev_list.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(pro_elev_list, jvalue))
			{
				LOG_ERROR("解析 pro_elev_list:%s json串错误", pro_elev_list.c_str());
				return false;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				vec_pro_elev.push_back(jvalue[i].asUInt());
				LOG_DEBUG("add pro elev:%d", jvalue[i].asUInt());
			}
		}

		CFishCfgMgr::Instance().ResetHitFishProEvelCfg(vec_pro_elev);
	}
	LOG_DEBUG("reader_json -  roomid:%d,tableid:%d m_map_list.size:%d m_bot_list.size:%d m_arm_range_list.size:%d vec_pro_elev.size:%d",
		GetRoomID(), GetTableID(), m_map_list.size(), m_bot_list.size(), m_arm_range_list.size(), vec_pro_elev.size());
	return true;
}

void CGameFishingTable::ShutDown()
{

}

//结束游戏，复位桌子
void CGameFishingTable::ResetTable()
{
       
}

//精度为10ms一次
void CGameFishingTable::OnTimeTick()
{
	//游戏开始
	if(m_coolLogic.isTimeOut())
    {
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_FISHING_START:
            {               
            }
			break;		
		case TABLE_STATE_FISHING_PAUSE:
			{
				OnGameStart();
			}break;       
        default:
            break;
        }
    }    

	//判断桌子是否开始
	if (GetGameState()== TABLE_STATE_FISHING_PAUSE)
	{
		return;		
	}		

	//判断所有鱼是否都已经生成
	//if(!m_create_fish_end)
	{
		OnCreateFishData();
	}

	//当前地图是否循环
	if (m_map_cycle_timer.isTimeOut())
	{
		m_map_cycle_timer.clearCool();

		//清空上一地图的所有鱼信息
		m_all_fish_info.clear();

		//群发消息
		net::msg_fishing_notice_change_map notice_msg;
		m_map_pos++;
		m_map_level = GetMapID();
		notice_msg.set_map_id(m_map_level);
		SendMsgToPlayer(&notice_msg, net::S2C_MSG_FISHING_NOTICE_CHANGE_MAP);

		m_start_time = getTickCount64();

		//根据地图ID获取对应的地图数据
		m_map_cost_time = 0;
		m_curr_map_info.clear();
		bool ret = CFishCfgMgr::Instance().GetCurrMapInfo(m_map_level, m_map_cost_time, m_curr_map_info);
		if (!ret)
		{
			LOG_ERROR("get curr map info is fail. m_map_level:%d", m_map_level);
			return;
		}
		else
		{
			m_map_cycle_count++;
			m_map_cycle_timer.beginCooling(m_map_cost_time);			// 地图循环定时器
			m_create_fish_end = false;

			LOG_DEBUG("switch map room_id:%d table_id:%d count:%d m_map_level:%d m_map_pos:%d m_map_cost_time:%d m_curr_map_info.size:%d",
				GetRoomID(), GetTableID(), m_map_cycle_count, m_map_level, m_map_pos, m_map_cost_time, m_curr_map_info.size());
		}	
		
		//清空所有玩家的子弹编号
		ZeroMemory(m_lTableBulletID, sizeof(m_lTableBulletID));		
	}

	//玩家战绩更新
	if (m_score_timer.isTimeOut())
	{
		OnSynDBAndRedis();
		m_player_novicectrlinfo.clear();
		m_score_timer.beginCooling(SCORE_TIME);		
	}

	//清除无效鱼---超出地图范围
	if (m_clear_fish_timer.isTimeOut())
	{
		OnClearInvalidFish();
		m_clear_fish_timer.beginCooling(CLEAR_FISH_TIME);
	}

	//清除长时间不操作的玩家
	uint64 curr_time = getSysTime();
	for (int i = 0; i < GAME_PLAYER; i++)
	{
		if (curr_time - m_MsgTime[i] > SECONDS_IN_MIN)
		{
			CGamePlayer* pGamePlayer = GetPlayer(i);
			if (pGamePlayer != NULL)
			{
				LOG_DEBUG("kick out long time no operator player room_id:%d table_id:%d uid:%d", GetRoomID(), GetTableID(), pGamePlayer->GetUID());
				pGamePlayer->SetNetState(0);
			}	
			m_MsgTime[i] = curr_time;
		}
	}
}

// 游戏消息
int CGameFishingTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if (pPlayer == NULL)
	{
		return 0;
	}

	uint32 uid = pPlayer->GetUID();
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("OnGameMessage - roomid:%d,tableid:%d,uid:%d,chairID:%d,cmdID:%d", GetRoomID(), GetTableID(), uid, chairID, cmdID);
	m_MsgTime[chairID] = getSysTime();

	switch(cmdID)
    {
    case net::C2S_MSG_FISHING_GAME_INFO_REQ:			// 游戏信息请求
        {
			net::msg_fishing_game_info_rsp rep;
			uint64 curr_time = getTickCount64();		// 获取当前系统时间(毫秒)
			rep.set_interval_time(curr_time - m_start_time);
			for (size_t i = 0; i < m_all_fish_info.size(); i++)
			{
				if (!m_all_fish_info[i].fish_dead)
				{
					rep.add_fish_no(m_all_fish_info[i].fish_no);
					LOG_DEBUG("SendGameScene - live fish_no:%d", m_all_fish_info[i].fish_no);
				}
			}
			rep.set_map_id(m_map_level);
			rep.set_bullet_id(m_lTableBulletID[chairID]);
			for (size_t i = 0; i < m_bot_list.size(); i++)
			{
				rep.add_bot_list(m_bot_list[i]);
				LOG_DEBUG("SendGameScene - bot:%d", m_bot_list[i]);
			}
			for (size_t i = 0; i < m_arm_range_list.size(); i++)
			{
				rep.add_arm_range(m_arm_range_list[i]);
				LOG_DEBUG("SendGameScene - arm range:%d", m_arm_range_list[i]);
			}
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_FISHING_GAME_INFO_RSP);
			LOG_DEBUG("SendGameScene interval_time:%llu fish count:%d", curr_time - m_start_time, rep.fish_no_size());

        }break;
    case net::C2S_MSG_FISHING_SHOOTING_REQ:				// 玩家射击
        {
            if(GetGameState() != TABLE_STATE_FISHING_START)
                return 0;

			//请求消息
			net::msg_fishing_shooting_req req_msg;
			PARSE_MSG_FROM_ARRAY(req_msg);

			//判断座位号是否正确
			if (req_msg.seat_id() >= GAME_PLAYER)
				return 0;

			//判断当前玩家的钱是否足够
			int64 player_score = GetPlayerCurScore(pPlayer); //pPlayer->GetAccountValue(emACC_VALUE_COIN); modify by har
			int64 curr_score = player_score + m_lTableScore[req_msg.seat_id()] - req_msg.bullet_bot();
			if (curr_score < 0)
			{
				LOG_DEBUG("the player is not money uid:%d curr_score:%lld", uid, curr_score);
				return 0;
			}

			//更新玩家金币值
			m_lTableScore[req_msg.seat_id()] -= req_msg.bullet_bot();
			m_lTableBulletID[req_msg.seat_id()] = req_msg.bullet_id();
			LOG_DEBUG("shooting seat_id:%d bullet_id:%d bullet_direct:%d bullet_bot:%d table_score:%lld fish_no:%d", 
				req_msg.seat_id(), req_msg.bullet_id(), req_msg.bullet_direct(), req_msg.bullet_bot(), m_lTableScore[req_msg.seat_id()], req_msg.fish_no());

			//统计玩家子弹分类
			map<uint32, map<uint32, uint32>>::iterator iter_seat = m_bullet_classification.find(req_msg.seat_id());
			if (iter_seat != m_bullet_classification.end())
			{
				map<uint32, uint32>::iterator iter_bot = iter_seat->second.find(req_msg.bullet_bot());
				if (iter_bot != iter_seat->second.end())
				{
					iter_bot->second++;
				}
				else
				{
					iter_seat->second[req_msg.bullet_bot()] = 1;
				}
			}
			else
			{
				map<uint32, uint32> tmp_map;
				tmp_map[req_msg.bullet_bot()] = 1;
				m_bullet_classification[req_msg.seat_id()] = tmp_map;
			}

			//群发消息
			net::msg_fishing_notice_shooting notice_msg;
			notice_msg.set_seat_id(req_msg.seat_id());
			notice_msg.set_bullet_id(req_msg.bullet_id());
			notice_msg.set_bullet_direct(req_msg.bullet_direct());
			notice_msg.set_bullet_bot(req_msg.bullet_bot());
			notice_msg.set_fish_no(req_msg.fish_no());
			notice_msg.set_score(curr_score);
			LOG_DEBUG("BBB shooting uid:%d score:%lld seat_id:%d player_score:%lld table_score:%lld", uid, notice_msg.score(), req_msg.seat_id(), player_score, m_lTableScore[req_msg.seat_id()]);

			SendMsgToPlayer(&notice_msg, net::S2C_MSG_FISHING_SHOOTING_RSP);
				           
        }break;
    case net::C2S_MSG_FISHING_HIT_REQ:			// 击中目标请求
        {                        
			net::msg_fishing_hit_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			OnUserHitFish(chairID, msg.bullet_id(), msg.bullet_bot(), msg.fish_no(), msg.timestamp());
			return 0;
        }break;   
	case net::C2S_MSG_FISHING_CHANGE_BOT_REQ:	// 玩家修改底分请求
		{
			//请求消息
			net::msg_fishing_change_bot req_msg;
			PARSE_MSG_FROM_ARRAY(req_msg);		

			LOG_DEBUG("change bot seat_id:%d bot:%d", req_msg.seat_id(), req_msg.bot());

			//群发消息
			net::msg_fishing_notice_change_bot notice_msg;
			notice_msg.set_seat_id(req_msg.seat_id());
			notice_msg.set_bot(req_msg.bot());
			SendMsgToPlayer(&notice_msg, net::S2C_MSG_FISHING_CHANGE_BOT_RSP);

		}break;
	case net::C2S_MSG_FISHING_USE_PROP_REQ:	// 玩家使用道具请求
		{
			//请求消息
			net::msg_fishing_use_prop req_msg;
			PARSE_MSG_FROM_ARRAY(req_msg);

			LOG_DEBUG("use prop uid:%d seat_id:%d prop_id:%d state:%d", uid, req_msg.seat_id(), req_msg.prop_id(), req_msg.state());
			
			//群发消息
			net::msg_fishing_notice_use_prop notice_msg;
			notice_msg.set_seat_id(req_msg.seat_id());
			notice_msg.set_prop_id(req_msg.prop_id());
			notice_msg.set_state(req_msg.state());
			SendMsgToPlayer(&notice_msg, net::S2C_MSG_FISHING_NOTICE_USE_PROP);

		}break;
	case net::C2S_MSG_FISHING_MAP_INFO_REQ:			// 地图信息请求
		{
			net::msg_fishing_map_info_rsp rsp;
			uint64 curr_time = getTickCount64();		// 获取当前系统时间(毫秒)
			rsp.set_interval_time(curr_time - m_start_time);			
			pPlayer->SendMsgToClient(&rsp, net::S2C_MSG_FISHING_MAP_INFO_RSP);
			LOG_DEBUG("SendGameScene interval_time:%llu", curr_time - m_start_time);

		}break;
	case net::C2S_MSG_FISHING_BULLET_TRACE_REQ:	// 玩家子弹跟踪请求
		{
			//请求消息
			net::msg_fishing_bullet_trace_req req_msg;
			PARSE_MSG_FROM_ARRAY(req_msg);

			LOG_DEBUG("use bullet trace seat_id:%d fish_no:%d", req_msg.seat_id(), req_msg.fish_no());

			//群发消息
			net::msg_fishing_notice_bullet_trace notice_msg;
			notice_msg.set_seat_id(req_msg.seat_id());
			notice_msg.set_fish_no(req_msg.fish_no());
			SendMsgToPlayer(&notice_msg, net::S2C_MSG_FISHING_NOTICE_BULLET_TRACE);

		}break;
	case net::C2S_MSG_FISHING_NEW_BULLET_REQ:	// 玩家生成新子弹请求
		{
			//请求消息
			net::msg_fishing_new_bullet_req req_msg;
			PARSE_MSG_FROM_ARRAY(req_msg);

			LOG_DEBUG("player new bullet req. seat_id:%d bullet_id:%d bullet_direct:%d bullet_bot:%d fish_no:%d pos_x:%d pos_y:%d", 
				req_msg.seat_id(), req_msg.bullet_id(), req_msg.bullet_direct(), req_msg.bullet_bot(), req_msg.fish_no(), req_msg.pos_x(), req_msg.pos_y());

			//群发消息
			net::msg_fishing_notice_new_bullet notice_msg;
			notice_msg.set_seat_id(req_msg.seat_id());
			notice_msg.set_bullet_id(req_msg.bullet_id());
			notice_msg.set_bullet_direct(req_msg.bullet_direct());
			notice_msg.set_bullet_bot(req_msg.bullet_bot());
			notice_msg.set_fish_no(req_msg.fish_no());
			notice_msg.set_pos_x(req_msg.pos_x());
			notice_msg.set_pos_y(req_msg.pos_y());
			SendMsgToPlayer(&notice_msg, net::S2C_MSG_FISHING_NOTICE_NEW_BULLET);

		}break;
    default:
        return 0;
    }
    return 0;
}

// 游戏开始
bool CGameFishingTable::OnGameStart()
{  
    //设置状态
    SetGameState(TABLE_STATE_FISHING_START);
			
	m_coolLogic.beginCooling(FRUSH_TIME);					//30毫秒一次 
	m_score_timer.beginCooling(SCORE_TIME);					//玩家战绩定时器
	m_clear_fish_timer.beginCooling(CLEAR_FISH_TIME);		//清除地图无效鱼定时器
						
	//设置初始地图
	m_map_pos = GetRoomID();
	m_map_level = GetMapID();

	//重置金流日志
	InitBlingLog(GAME_PLAYER);

	//根据地图ID获取对应的地图数据
	m_map_cost_time = 0;
	m_curr_map_info.clear();
	bool ret = CFishCfgMgr::Instance().GetCurrMapInfo(m_map_level, m_map_cost_time, m_curr_map_info);
	if (!ret)
	{		
		LOG_ERROR("get curr map info is fail. m_map_level:%d", m_map_level);
		return false;
	}
	else
	{
		m_map_cycle_timer.beginCooling(m_map_cost_time);			// 地图循环定时器
	}

	//取系统时间
	m_start_time = getTickCount64();
	LOG_DEBUG("gamestart roomid:%d,tableid:%d,start_time:%llu m_map_level:%d m_map_cost_time:%d m_curr_map_info.size:%d", 
		GetRoomID(), GetTableID(), m_start_time, m_map_level, m_map_cost_time, m_curr_map_info.size());

	return true;
}

//玩家进入或离开
void  CGameFishingTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	LOG_DEBUG("OnPlayerJoin - roomid:%d tableid:%d isJoin:%d chairID:%d", GetRoomID(), GetTableID(), isJoin, chairID);

	//如果加入，则提前调用基类方法，如果为退出，则需要在更新完金币后，再调用基类方法，保证金币一致
	if (isJoin)
	{
		CGameTable::OnPlayerJoin(isJoin, chairID, pPlayer);
	}
         
    if(isJoin)	//玩家加入
	{
        SendGameScene(pPlayer);	
		m_MsgTime[chairID] = getSysTime();
		LOG_DEBUG("player join table - roomid:%d tableid:%d uid:%d chairID:%d join_time:%llu", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID, m_MsgTime[chairID]);
		
    }	
	else        //玩家退出
	{
		//LOG_DEBUG("OnPlayerJoin - roomid:%d tableid:%d uid:%d chairID:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID);
		if (pPlayer != NULL && chairID < GAME_PLAYER)
		{
			//LOG_DEBUG("OnPlayerJoin - roomid:%d tableid:%d uid:%d table_score:%lld", GetRoomID(), GetTableID(), pPlayer->GetUID(), m_lTableScore[chairID]);
			if (m_lTableScore[chairID] != 0)
			{
				OnLeaveSynDBAndRedis(pPlayer, chairID);
			}			
		}

		m_lTableBulletID[chairID] = 0;

		CGameTable::OnPlayerJoin(isJoin, chairID, pPlayer);
	}

	//重置座位玩家战绩
	if (chairID < GAME_PLAYER)
	{
		m_lTableScore[chairID] = 0;
	}
}

// 发送场景信息(断线重连)
void CGameFishingTable::SendGameScene(CGamePlayer* pPlayer)
{
    uint16 chairID = GetChairID(pPlayer);
	
	uint64 curr_time = getTickCount64();		//获取当前系统时间(毫秒)
	net::msg_fishing_game_info_rsp rep;	
	rep.set_interval_time(curr_time - m_start_time);
	for (size_t i = 0; i < m_all_fish_info.size(); i++)
	{
		if (!m_all_fish_info[i].fish_dead)
		{
			rep.add_fish_no(m_all_fish_info[i].fish_no);
			LOG_DEBUG("SendGameScene - live fish_no:%d", m_all_fish_info[i].fish_no);
		}
	}	
	rep.set_map_id(m_map_level);
	rep.set_bullet_id(m_lTableBulletID[chairID]);
	for (size_t i = 0; i < m_bot_list.size(); i++)
	{
		rep.add_bot_list(m_bot_list[i]);
		LOG_DEBUG("SendGameScene - bot:%d", m_bot_list[i]);
	}
	for (size_t i = 0; i < m_arm_range_list.size(); i++)
	{
		rep.add_arm_range(m_arm_range_list[i]);
		LOG_DEBUG("SendGameScene - arm range:%d", m_arm_range_list[i]);
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_FISHING_GAME_INFO_RSP);
	LOG_DEBUG("SendGameScene - roomid:%d tableid:%d uid:%d chairID:%d interval_time:%llu fish count:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), chairID, curr_time - m_start_time, rep.fish_no_size());
}

//击中目标
void CGameFishingTable::OnUserHitFish(uint32 seat_id, uint32 bullet_id, uint32 bullet_bot, uint32 fish_no, uint64 time_stamp)
{
	LOG_DEBUG("OnUserHitFish - roomid:%d,tableid:%d,uid:%d,seat_id:%d bullet_id:%d bullet_bot:%d fish_no:%d time_stamp:%llu", 
		GetRoomID(), GetTableID(), GetPlayerID(seat_id), seat_id, bullet_id, bullet_bot, fish_no, time_stamp);
	
	if (seat_id >= GAME_PLAYER)
	{
		LOG_DEBUG("seat_id is error.");
		return;
	}

	CGamePlayer *pPlayer = GetPlayer(seat_id);
	if (pPlayer == NULL)
	{
		LOG_DEBUG("get player is fail.");
		return;
	}

	//根据鱼的编号查找鱼的种类
	vector<FishInfo>::iterator iter = m_all_fish_info.begin();
	for (; iter != m_all_fish_info.end(); iter++)
	{
		//匹配鱼的编号
		if (iter->fish_no == fish_no)
		{
			//判断当前鱼是否已死
			if (iter->fish_dead)
			{
				net::msg_fishing_hit_no_fish_rsp msg;
				msg.set_seat_id(seat_id);
				msg.set_bullet_id(bullet_id);
				msg.set_bullet_bot(bullet_bot);
				msg.set_fish_id(fish_no);	
				msg.set_timestamp(time_stamp);
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_FISHING_HIT_NO_FISH_RSP);

				LOG_DEBUG("The current fish was dead. - roomid:%d tableid:%d uid:%d fish_no:%d", GetRoomID(), GetTableID(), GetPlayerID(seat_id), fish_no);
				break;
			}

			uint32 fish_type = iter->fish_type;
			uint32 ret = 0;					//击中鱼的倍数
			bool is_luck_ctrl = false;		//是否控制幸运值
			bool is_luck_win = false;		//幸运值的输赢标志
			bool is_aw_ctrl = false;		//是否活跃福利控制
			bool is_novice_ctrl = false;	//是否新手福利控制

			bool flag = CFishCfgMgr::Instance().GetFishCfgInfo(m_pHostRoom, pPlayer, fish_type, ret, is_luck_ctrl, is_luck_win, is_aw_ctrl, is_novice_ctrl);

			//如果当前鱼已死
			if (flag)
			{
				//设置鱼的状态
				iter->fish_dead = true;

				//计算当前击中鱼的得分
				int64 add_value = bullet_bot * ret;

				//扣除抽水
				DeducEndFee(pPlayer->GetUID(), add_value);

				m_lTableScore[seat_id] += add_value;

				net::msg_fishing_hit_rsp msg;
				msg.set_seat_id(seat_id);
				msg.set_bullet_id(bullet_id);
				msg.set_bullet_bot(bullet_bot);
				msg.set_fish_id(fish_no);
				msg.set_fish_multi(ret);
				int64 player_score = GetPlayerCurScore(pPlayer); // pPlayer->GetAccountValue(emACC_VALUE_COIN); modify by har
				msg.set_score(player_score + m_lTableScore[seat_id]);
			
				LOG_DEBUG("BBB kill fish success - uid:%d seat_id:%d fish_type:%d ret:%d add_value:%d table_score:%lld set_score:%lld is_novice_ctrl:%d is_aw_ctrl:%d", 
					pPlayer->GetUID(), seat_id, fish_type, ret, add_value, m_lTableScore[seat_id], player_score + m_lTableScore[seat_id], is_novice_ctrl, is_aw_ctrl);

				SendMsgToPlayer(&msg, net::S2C_MSG_FISHING_HIT_RSP);

				//幸运值的操作记录
				if (is_luck_ctrl && is_luck_win)
				{
					pPlayer->SetLuckyInfo(GetRoomID(), m_lTableScore[seat_id]);
				}

				//活跃福利的操作记录
				if (is_aw_ctrl)
				{
					UpdateActiveWelfareInfo(pPlayer->GetUID(), m_lTableScore[seat_id]);					
				}

				//新手福利的操作记录
				if (is_novice_ctrl)
				{
					map<uint32, NoviceCtrlInfo>::iterator iter = m_player_novicectrlinfo.find(pPlayer->GetUID());
					if (iter == m_player_novicectrlinfo.end())
					{
						NoviceCtrlInfo info;
						info.times = 1;
						info.win_socre = add_value;
						m_player_novicectrlinfo[pPlayer->GetUID()] = info;
						LOG_DEBUG("add novice_ctrl info uid:%d win_socre:%d", pPlayer->GetUID(), add_value);
					}
					else
					{
						iter->second.times++;
						iter->second.win_socre += add_value;
						LOG_DEBUG("update novice_ctrl info uid:%d times:%d win_socre:%d", pPlayer->GetUID(), iter->second.times, iter->second.win_socre);
					}

					//更新福利局数
					pPlayer->UpdateWelCount(1);
				}

				//统计玩家打死鱼的分类
				map<uint32, map<uint32, KillFishInfo>>::iterator iter_seat = m_kill_classification.find(seat_id);
				if (iter_seat != m_kill_classification.end())
				{
					map<uint32, KillFishInfo>::iterator iter_bot = iter_seat->second.find(fish_type);
					if (iter_bot != iter_seat->second.end())
					{
						iter_bot->second.add_value += add_value;
						iter_bot->second.times++;
					}
					else
					{
						KillFishInfo info;
						info.add_value = add_value;
						info.times = 1;
						iter_seat->second[fish_type] = info;
					}
				}
				else
				{
					KillFishInfo info;
					info.add_value = add_value;
					info.times = 1;
					map<uint32, KillFishInfo> tmp_map;
					tmp_map[fish_type] = info;
					m_kill_classification[seat_id] = tmp_map;
				}				
			}
			else    //鱼未死
			{
				//幸运值的操作记录---控制输的情况，只记录子弹消耗
				if (is_luck_ctrl && !is_luck_win)
				{
					pPlayer->SetLuckyInfo(GetRoomID(), -bullet_bot);
				}
			}
			break;
		}
	}
	if (iter == m_all_fish_info.end())
	{
		LOG_DEBUG("The fish is not exist map. - roomid:%d tableid:%d uid:%d fish_no:%d",
			GetRoomID(), GetTableID(), GetPlayerID(seat_id), fish_no);
	}
	return;
}


//判断是否产生新鱼---定时器调用
void  CGameFishingTable::OnCreateFishData()
{
	//LOG_DEBUG("OnCreateFishData - roomid:%d tableid:%d map_level:%d", GetRoomID(), GetTableID(), m_map_level);
	uint64 curr_time = getTickCount64();		//获取当前系统时间(毫秒)
	//m_create_fish_end = true;

	//遍历当前地图的所有线路
	vector<CurrMapInfo>::iterator iter_line = m_curr_map_info.begin();
	for (; iter_line != m_curr_map_info.end(); iter_line++)
	{
		uint32 line_id = iter_line->id;		//当前线路ID
		if (iter_line->create_info.is_create && curr_time >= iter_line->create_info.next_create_time)
		{
			//针对walkLines的处理
			if (!iter_line->is_single)
			{
				//判断鱼的数量
				if (iter_line->count != 0 && iter_line->create_info.fish_curr >= iter_line->count)
				{
					iter_line->create_info.is_create = false;
					continue;
				}

				//生成鱼的信息
				uint32 fish_serial = 0;
				uint64 fish_no = 0;
				uint64 fish_deadtime = 0;

				if (iter_line->create_info.fish_serial >= MAX_FISH_DIRECTORY - 1)
				{
					iter_line->create_info.fish_serial = 1;
				}
				else
				{
					iter_line->create_info.fish_serial++;
				}
				fish_serial = iter_line->create_info.fish_serial;
				fish_no = line_id * MAX_FISH_DIRECTORY + fish_serial;
				fish_deadtime = curr_time + iter_line->wl_cost_time;

				FishInfo fish_info;
				fish_info.createTime = curr_time;		
				fish_info.fish_no = fish_no;
				fish_info.fish_type = iter_line->fishID;
				fish_info.deadTime = fish_deadtime;
				m_all_fish_info.push_back(fish_info);

				//由于定时器误差的补偿时间
				uint64 reimburse = curr_time - iter_line->create_info.next_create_time;		

				//重置定时器
				iter_line->create_info.fish_curr++;
				iter_line->create_info.next_create_time = curr_time + iter_line->fishWaitTime - reimburse;
				iter_line->create_info.is_create = true;	

				LOG_DEBUG("create walkLines fish success. roomid:%d tableid:%d fish_no:%d curr:%d fish_serial:%d fishWaitTime:%d fish_deadtime:%llu curr_time:%llu next_create_time:%llu",
					GetRoomID(), GetTableID(), fish_no, iter_line->create_info.fish_curr, fish_serial, iter_line->fishWaitTime, fish_deadtime, curr_time, iter_line->create_info.next_create_time);

				m_create_fish_end = false;
			}
			else    //针对fishSet的处理
			{
				for (size_t i=0; i<iter_line->fs_list.size(); i++)
				{
					uint32 fish_serial = iter_line->fs_list[i].id;
					uint32 fish_cost_time = iter_line->fs_list[i].cost_time;
					uint32 fish_no = line_id * MAX_FISH_DIRECTORY + fish_serial;

					FishInfo fish_info;
					fish_info.createTime = curr_time;		//获取当前系统时间(毫秒)
					fish_info.fish_no = fish_no;
					fish_info.fish_type = iter_line->fishID;
					fish_info.deadTime = curr_time + fish_cost_time;
					m_all_fish_info.push_back(fish_info);

					m_create_fish_end = false;
					LOG_DEBUG("create fishSet fish success. roomid:%d tableid:%d fish_no:%d deadTime:%llu", GetRoomID(), GetTableID(), fish_no, fish_info.deadTime);
				}

				//因为此类型的鱼只产生一次，所以产生完就删除
				iter_line->fs_list.clear();
			}
		}			
	}
}

//清除地图上无效鱼信息---定时器调用
void	CGameFishingTable::OnClearInvalidFish()
{
	//LOG_DEBUG("OnClearInvalidFish - roomid:%d tableid:%d", GetRoomID(), GetTableID());
	uint64 curr_time = getTickCount64();		//获取当前系统时间(毫秒)
	vector<FishInfo>::iterator iter = m_all_fish_info.begin();
	for (; iter != m_all_fish_info.end();)
	{		
		if (curr_time > iter->deadTime)	
		{
			LOG_DEBUG("delete out time fish roomid:%d tableid:%d fish no:%d curr_time:%lld createTime:%lld dead_time:%lld", 
				GetRoomID(), GetTableID(), iter->fish_no, curr_time, iter->createTime, iter->deadTime);
			iter = m_all_fish_info.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

// 重置金流日志
void CGameFishingTable::InitBlingLog(uint32 seat_id)
{
	m_chessid = CStringUtility::FormatToString("%d-%d-%d-%llu", m_pHostRoom->GetGameType(), m_pHostRoom->GetRoomID(), GetTableID(), getTickCount64());

	m_blingLog.Reset();
	m_blingLog.baseScore = m_conf.baseScore;
	m_blingLog.consume = m_conf.consume;
	m_blingLog.deal = m_conf.deal;
	m_blingLog.startTime = getSysTime();
	m_blingLog.gameType = m_pHostRoom->GetGameType();
	m_blingLog.roomType = m_pHostRoom->GetRoomType();
	m_blingLog.tableID = GetTableID();
	m_blingLog.roomID = m_pHostRoom->GetRoomID();
	m_blingLog.chessid = m_chessid;
	if (m_player_novicectrlinfo.size() > 0)
	{
		m_blingLog.welctrl = 1;
		m_blingLog.welfare = 1;
	}
	else
	{
		m_blingLog.welctrl = 0;
		m_blingLog.welfare = 0;
	}
	
	m_blingLog.users.clear();

	if (seat_id >= GAME_PLAYER)
	{
		for (uint16 i = 0; i < m_vecPlayers.size(); ++i) {
			stBlingUser user;
			CGamePlayer* pPlayer = m_vecPlayers[i].pPlayer;
			if (pPlayer == NULL)
			{
				continue;
			}
			user.uid = pPlayer->GetUID();
			user.playerType = pPlayer->GetPlayerType();
			user.oldValue = GetPlayerCurScore(pPlayer);
			user.safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
			user.chairid = i;
			user.totalwinc = GetPlayerTotalWinScore(pPlayer);
			user.stockscore = pPlayer->GetPlayerStockScore(GetGameType());
			user.gamecount = pPlayer->GetPlayerGameCount(GetGameType());

			//记录新手福利次数与金额
			map<uint32, NoviceCtrlInfo>::iterator iter = m_player_novicectrlinfo.find(pPlayer->GetUID());
			if (iter != m_player_novicectrlinfo.end())
			{
				user.welfare = 1;
				user.fish_novice_times = iter->second.times;
				user.fish_novice_win_socre = iter->second.win_socre;
			}

			m_blingLog.users.push_back(user);
		}
	}
	else
	{
		stBlingUser user;
		CGamePlayer* pPlayer = m_vecPlayers[seat_id].pPlayer;
		if (pPlayer != NULL)
		{
			user.uid = pPlayer->GetUID();
			user.playerType = pPlayer->GetPlayerType();
			user.oldValue = GetPlayerCurScore(pPlayer);
			user.safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
			user.chairid = seat_id;
			user.totalwinc = GetPlayerTotalWinScore(pPlayer);
			user.stockscore = pPlayer->GetPlayerStockScore(GetGameType());
			user.gamecount = pPlayer->GetPlayerGameCount(GetGameType());

			//记录新手福利次数与金额
			map<uint32, NoviceCtrlInfo>::iterator iter = m_player_novicectrlinfo.find(pPlayer->GetUID());
			if (iter != m_player_novicectrlinfo.end())
			{
				user.welfare = 1;
				user.fish_novice_times = iter->second.times;
				user.fish_novice_win_socre = iter->second.win_socre;
			}

			m_blingLog.users.push_back(user);
		}		
	}	
	m_operLog.clear();
}

// 定时写入金流log
void  CGameFishingTable::WriteAddScoreLog(uint32 uid, int64 win, uint32 seat_id)
{
	if (win == 0)
		return;

	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["win"] = win;

	//添加射击详情
	map<uint32, map<uint32, uint32>>::iterator iter_seat = m_bullet_classification.find(seat_id);
	if (iter_seat != m_bullet_classification.end())
	{
		map<uint32, uint32>::iterator iter_bot = iter_seat->second.begin();
		for (; iter_bot != iter_seat->second.end(); iter_bot++)
		{
			Json::Value jbot;
			jbot["bot"] = iter_bot->first;
			jbot["times"] = iter_bot->second;
			logValue["shoot_detail"].append(jbot);
			iter_bot->second++;
		}		
	}	

	//添加打死鱼的详情
	map<uint32, map<uint32, KillFishInfo>>::iterator iter_pos = m_kill_classification.find(seat_id);
	if (iter_pos != m_kill_classification.end())
	{
		map<uint32, KillFishInfo>::iterator iter_bot = iter_pos->second.begin();
		for (; iter_bot != iter_pos->second.end(); iter_bot++)
		{
			Json::Value jbot;
			jbot["fish_type"] = iter_bot->first;
			jbot["add_value"] = iter_bot->second.add_value;
			jbot["times"] = iter_bot->second.times;
			logValue["kill_detail"].append(jbot);			
		}
	}

	m_operLog["op"].append(logValue);
}

//定时同步数据到数据库及redis
void    CGameFishingTable::OnSynDBAndRedis()
{
	//LOG_DEBUG("OnSynDBAndRedis - roomid:%d,tableid:%d", GetRoomID(), GetTableID());

	//重置金流日志
	InitBlingLog(GAME_PLAYER);

	bool need_record = false;
	//用户设置
	for (WORD i = 0; i < GAME_PLAYER; i++)
	{
		//获取用户
		CGamePlayer *pPlayer = GetPlayer(i);
		if (pPlayer == NULL || m_lTableScore[i] == 0)
			continue;

		//更新玩家信息
		CalcPlayerGameInfoForFish(pPlayer->GetUID(), m_lTableScore[i]);

		//记录金流日志
		WriteAddScoreLog(pPlayer->GetUID(), m_lTableScore[i], i);

		if (!pPlayer->IsRobot()) {
			SetIsAllRobotOrPlayerJetton(false);
			m_pHostRoom->UpdateStock(this, m_lTableScore[i]); // add by har
		}
		m_lTableScore[i] = 0;

		need_record = true;
	}
	
	//同步金流日志到redis
	if (need_record)
	{
		SaveBlingLog();		
	}	

	//清空子弹分类统计
	m_bullet_classification.clear();
	m_kill_classification.clear();
}

//获取地图ID
uint32	CGameFishingTable::GetMapID()
{
	if (m_map_list.size() == 0)
	{
		LOG_ERROR("The room config map list is empty. roomid:%d,tableid:%d", GetRoomID(), GetTableID());
		return 0;
	}

	if (m_map_pos >= m_map_list.size())
	{
		m_map_pos = 0;
	}
	return m_map_list[m_map_pos];
}

//玩家退出同步数据到数据库及redis
void    CGameFishingTable::OnLeaveSynDBAndRedis(CGamePlayer* pPlayer, uint32 seat_id)
{
	LOG_DEBUG("OnLeaveSynDBAndRedis - roomid:%d,tableid:%d uid:%d seat_id:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), seat_id);

	//更新玩家信息
	CalcPlayerGameInfoForFish(pPlayer->GetUID(), m_lTableScore[seat_id]);

	//重置金流日志
	InitBlingLog(seat_id);

	//记录金流日志
	WriteAddScoreLog(pPlayer->GetUID(), m_lTableScore[seat_id], seat_id);

	if (!pPlayer->IsRobot()) {
		SetIsAllRobotOrPlayerJetton(false);
		m_pHostRoom->UpdateStock(this, m_lTableScore[seat_id]); // add by har
	}
	m_lTableScore[seat_id] = 0;

	//同步金流日志到redis
	SaveBlingLog();

	//清除对应玩家的子弹分类统计
	map<uint32, map<uint32, uint32>>::iterator iter = m_bullet_classification.find(seat_id);
	if (iter != m_bullet_classification.end())
	{
		m_bullet_classification.erase(iter);
	}

	//清除对应玩家的打死鱼统计
	map<uint32, map<uint32, KillFishInfo>>::iterator iter_kill = m_kill_classification.find(seat_id);
	if (iter_kill != m_kill_classification.end())
	{
		m_kill_classification.erase(iter_kill);
	}
}