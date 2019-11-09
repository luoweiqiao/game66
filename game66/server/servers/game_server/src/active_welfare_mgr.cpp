//
// Created by Joe on 2019-01-26 活跃玩家管理类
//

#include "active_welfare_mgr.h"
#include "stdafx.h"
#include "data_cfg_mgr.h"
#include "game_define.h"

using namespace std;

CAcTiveWelfareMgr::CAcTiveWelfareMgr()
{
    m_mpAWCfg.clear();
}
CAcTiveWelfareMgr::~CAcTiveWelfareMgr()
{
    m_mpAWCfg.clear();
}

bool CAcTiveWelfareMgr::Init()
{
    //启动时加载活跃福利配置表
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadActiveWelfareCfg(m_mpAWCfg);
    if (!bRet)
    {
        return false;
    }  
    return true;
}

//加载配置数据
bool	CAcTiveWelfareMgr::ReLoadCfgData()
{
    //重新加载活跃福利配置表
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadActiveWelfareCfg(m_mpAWCfg);
    if (!bRet)
    {
        return false;
    }
    return true;
}

bool    CAcTiveWelfareMgr::GetPlayerActiveWelfareFlag(uint32 uid, uint64 loss_coin, uint8 game_id)
{    
    LOG_DEBUG("uid:%d loss_coin:%d game_id:%d", uid, loss_coin, game_id);
	
    CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pGamePlayer==NULL)
    {
        LOG_ERROR("get user info fail. uid:%", uid);
        return false;
    }

    int aw_id = 0;    //活跃福利ID
    map<uint8, stActiveWelfareCfg>::iterator iter = m_mpAWCfg.begin();
    for(;iter!= m_mpAWCfg.end();iter++)
    {
        LOG_DEBUG(" aw config info. id:%d min_loss:%d max_loss:%d", iter->first, iter->second.min_loss, iter->second.max_loss);

        //匹配亏损的区间范围
        if(iter->second.min_loss <= loss_coin && iter->second.max_loss > loss_coin)
        {
            //判断玩家在该区间是否已经完成补助
            uint64 aw_sum = pGamePlayer->GetCurrAWInfo(iter->first);
            if(aw_sum >= iter->second.welfare)
            {
                LOG_DEBUG(" the aw_sum more then welfare. uid:%d aw_sum:%d welfare:%d", uid, aw_sum, iter->second.welfare);
                return false;
            }
            else
            {
                aw_id = iter->first;
                break;
            }
        }
    }

    LOG_DEBUG(" get aw_id:%d aw_cfg_size:%d", aw_id, m_mpAWCfg.size());

    //判断玩家的亏损区间是否发生改变
    uint8 current_aw_id = pGamePlayer->GetCurrAWID();
    if(aw_id != current_aw_id)
    {
        LOG_DEBUG(" change aw id. uid:%d from_id:%d to_id:%d", uid, current_aw_id, aw_id);

        ////判断玩家在该区间是否已经完成补助 如果完成，则不能将累计值清零，如果没有完成，则需要将累计值清零
        //map<uint8, stActiveWelfareCfg>::iterator iter1 = m_mpAWCfg.find(current_aw_id);
        //if(iter1!= m_mpAWCfg.end())
        //{
        //    uint64 aw_sum = pGamePlayer->GetCurrAWInfo(current_aw_id);
        //    if (aw_sum < iter1->second.welfare)
        //    {
        //        LOG_DEBUG("the aw_sum less then welfare then clear aw_sum. uid:%d aw_sum:%llu welfare:%d", uid, aw_sum, iter1->second.welfare);
        //        pGamePlayer->ClearCurrAWInfo(pGamePlayer->GetCurrAWID());
        //    }
        //}        
        pGamePlayer->SetCurrAWID(aw_id);        
    }
	pGamePlayer->SetCurrAWBaseLoss(-loss_coin);

	return true;
}

bool    CAcTiveWelfareMgr::GetActiveWelfareCfgInfo(uint32 uid, uint8 aw_id, uint8 game_id, uint32 &max_win, uint32 &probability)
{
    LOG_DEBUG("GetActiveWelfareCfgInfo - uid:%d aw_id:%d game_id:%d max_win:%d probability:%d", uid, aw_id, game_id, max_win, probability);

	CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if (pGamePlayer == NULL)
    {
        LOG_ERROR("GetActiveWelfareCfgInfo - get user info fail. uid:%", uid);
        return false;
    }
       
    map<uint8, stActiveWelfareCfg>::iterator iter = m_mpAWCfg.find(aw_id);
   
    // 判断ID是否存在
    if (iter!= m_mpAWCfg.end())
    {
        //匹配游戏ID 先查找百人游戏 再查找对战游戏
        set<uint8>::iterator it = iter->second.multiplayer_games.find(game_id);
        if (it != iter->second.multiplayer_games.end())
        {
            probability = iter->second.multi_probability;            
			max_win = iter->second.max_win;
			return true;
        }
        else
        {
            set<uint8>::iterator it = iter->second.doublefight_games.find(game_id);
            if (it != iter->second.doublefight_games.end())
            {
                probability = iter->second.doublefight_probability;
				max_win = iter->second.max_win;
				return true;
            }
        }		
    }  
	//LOG_DEBUG("GetActiveWelfareCfgInfo is fail - uid:%d aw_id:%d game_id:%d max_win:%d probability:%d", uid, aw_id, game_id, max_win, probability);
    return false;  
}

// 记录活跃福利控制的牌局信息
void   CAcTiveWelfareMgr::WriteActiveWelfareLog(uint32 uid, uint8 aw_id, uint8 game_id, int32 win_coin)
{
    CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if (pGamePlayer == NULL)
    {
        LOG_ERROR("WriteActiveWelfareLog - get user info fail. uid:%", uid);
        return;
    }

    // 判断ID是否存在
    map<uint8, stActiveWelfareCfg>::iterator iter = m_mpAWCfg.find(aw_id);
    if (iter != m_mpAWCfg.end())
    {
        uint32 welfare = iter->second.welfare;
        uint64 current_loss = pGamePlayer->GetCurrAWInfo(aw_id);
		int64 base_loss = pGamePlayer->GetCurrAWBaseLoss();

        uint8 status = 0;  //阶段未完成
        if(current_loss>=welfare)
        {
            status = 1;    //阶段已完成
        }
        char buf[256];
        sprintf(buf, "'[%llu,%llu]'", iter->second.min_loss, iter->second.max_loss);
        LOG_DEBUG("WriteActiveWelfareLog - uid:%d game_id:%d base_loss:%lld loss_range:%s welfare:%d status:%d current_loss:%llu win_coin:%d", 
            uid, game_id, base_loss, buf, welfare, status, current_loss, win_coin);
        CDBMysqlMgr::Instance().InsertActiveWelfareLog(uid, game_id, base_loss, buf, welfare, status, current_loss, win_coin, getSysTime());
    }
}