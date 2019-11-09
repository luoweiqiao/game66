//
// Created by toney on 16/5/22.
//

#ifndef SERVER_GOBAL_ROBOT_MGR_H
#define SERVER_GOBAL_ROBOT_MGR_H

#include <player.h>
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "db_struct_define.h"
#include "robot_city.h"

class CPlayer;
class CLobbyRobot;

struct stRobotInfo
{
    stRobotCfg cfg;
};
class CGobalRobotMgr : public AutoDeleteSingleton<CGobalRobotMgr>
{
public:
    CGobalRobotMgr();
    ~CGobalRobotMgr();
    bool	Init();
    void	ShutDown();
    void	OnTimeTick();
    // 尝试登陆一个机器人
    bool    LoginRobot(stRobotCfg & refCfg);
	bool    LoginTimeRobot(stRobotCfg & refCfg);

    // 机器人存入积分财富币
    bool    StoreScoreCoin(CLobbyRobot* pPlayer,int64 score,int64 coin);
    // 机器人充值钻石
    bool    PayDiamond(CLobbyRobot* pPlayer,int64 diamond);
    // 兑换积分
    bool    TakeScoreCoin(CLobbyRobot* pPlayer,int64 score,int64 coin);

    // 获得机器人管理员
    CPlayer* GetRobotMgr(uint16 gameType);
    // 重新加载机器人数据
    void    ChangeAllRobotCfg();
    // 获得在线机器人数量
    uint32  GetRobotNum(uint16 gameType,uint8 level,uint32 batchid);
	uint32  GetRobotCountByBatchID(uint16 gameType, uint8 level, map<uint32, stRobotOnlineCfg> & mpBatchCount);

	bool    AddRobotPayPoolData(uint16 gameType);

	bool	UpdateLoadRobotStatus(uint32 gameType, bool flag);

	bool	CheckLoadRobotStatus(uint32 gameType);

	uint32  GetOnLineRobotCount(map<uint32, stRobotOnlineCfg> & mpBatchCount);

protected:
    bool    AddRobot(stRobotCfg & refCfg);


    // 检测新机器人上线
    void    CheckRobotLogin();

	void    CheckTimeRobotLogin();

    // 加载机器人充值池数据
    bool    LoadRobotPayPoolData();


protected:
    CCooling    m_checkLogin;
	CCooling    m_checkLoginEx;
    //map<uint32,stRobotOnlineCfg>    m_mpRobotCfg;
	int			m_iRobotCityCount;
	bool		m_bArrLoadAllDayRobotStatus[net::GAME_CATE_MAX_TYPE];
};







#endif //SERVER_GOBAL_ROBOT_MGR_H
