//
// Created by Joe on 2019-01-26 活跃玩家管理类
//

#ifndef ACTIVE_PLAYER_MGR_H
#define ACTIVE_PLAYER_MGR_H

#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "db_struct_define.h"

class CAcTiveWelfareMgr : public AutoDeleteSingleton<CAcTiveWelfareMgr>
{
public:
    CAcTiveWelfareMgr();
    ~CAcTiveWelfareMgr();

    // 初始化
    bool	Init();

    // 重新加载配置数据---来源于PHP通知
    bool	ReLoadCfgData();

    // 判断当前玩家是否得到活跃福利
    bool    GetPlayerActiveWelfareFlag(uint32 uid, uint64 loss_coin, uint8 game_id);

    // 获取当前活跃福利玩家的所在区间的配置值
    bool    GetActiveWelfareCfgInfo(uint32 uid, uint8 aw_id, uint8 game_id, uint32 &max_win, uint32 &probability);

    // 记录活跃福利控制的牌局信息
    void    WriteActiveWelfareLog(uint32 uid, uint8 aw_id, uint8 game_id, int32 win_coin);
    	
private: 

    map<uint8, stActiveWelfareCfg>    m_mpAWCfg;    //对应active_welfare_cfg配置表 活跃玩家福利表
	
};







#endif //ACTIVE_PLAYER_MGR_H
