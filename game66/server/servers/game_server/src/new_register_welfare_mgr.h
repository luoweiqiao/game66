//
// Created by AAA on 2019-03-04 新注册玩家管理类
//

#ifndef NEW_REGISTER_WELFARE_MGR_H
#define NEW_REGISTER_WELFARE_MGR_H

#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "db_struct_define.h"

class CNewRegisterWelfareMgr : public AutoDeleteSingleton<CNewRegisterWelfareMgr>
{
public:
	CNewRegisterWelfareMgr();
    ~CNewRegisterWelfareMgr();

    // 初始化
    bool	Init();

    // 重新加载配置数据---来源于PHP通知
    bool	ReLoadCfgData();

    // 获取新注册玩家配置值
    bool    IsExistGame(uint8 game_id);

	uint64    GetMinWinCoin() { return m_mpNrwCfg.min_win_coin; }
	uint64    GetMaxWinCoin() { return m_mpNrwCfg.max_win_coin; }

	uint64    GetTotalWin() { return m_mpNrwCfg.total_win; }
	uint64    GetMustWin() { return m_mpNrwCfg.must_win; }

	uint32    GetTotalWinRate() { return m_mpNrwCfg.total_win_rate; }
	uint32    GetPeriodDay() { return m_mpNrwCfg.period_day; }


    // 记录新注册玩家控制的牌局信息
    void    WriteNewRegisterWelfareLog(uint32 uid, uint8 game_id, uint32 curr_must_win, uint32 curr_total_win, uint64 curr_win_coin, uint64 total_win_coin, uint64 register_time);
    	
private: 

	stNewRegisterWelfareCfg   m_mpNrwCfg;    //对应new_register_welfare_cfg配置表 新注册玩家福利表
	
};

#endif //NEW_REGISTER_WELFARE_MGR_H
