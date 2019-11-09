//
// Created by toney on 16/5/29.
//

#ifndef SERVER_ROBOT_H
#define SERVER_ROBOT_H

#include "player.h"

class CLobbyRobot : public CPlayer
{
public:
    CLobbyRobot();
    ~CLobbyRobot();

    virtual bool 	OnLoginOut(uint32 leaveparam = 0);
    virtual void	OnGetAllData();
    virtual void 	OnTimeTick(uint64 uTime,bool bNewDay);
    // 是否需要回收
    virtual bool 	NeedRecover();

    // 返回大厅回调
    virtual void    BackLobby();
    // 游戏战报
    virtual void 	OnGameEnd(uint16 gameType);

public:
    void    SetRobotCfg(stRobotCfg& cfg);
    int64   GetRichLvScore();
    int64   GetRichLvCoin();
    int64   GetLoginOutScore();
    int64   GetLoginOutCoin();
    uint16  GetGameType();
    uint16  GetCfgLevel();
	uint16  GetLoginType();
    bool    CanEnterGameType(uint16 gameType);

    void    SetNeedLoginOut(bool bNeed){ m_needLoginOut=bNeed; }

    // 获得机器人是否需要存入积分财富币
    bool    IsNeedStore(int64 &score,int64 &coin);

    // 需要兑换的积分
    bool    IsNeedExchange(int64 &score,int64 &coin);

    // 获得机器人存取保险箱财富币
    bool    IsNeedTakeSafeBox(int64 &coin);
    // 处理金币存取
    bool    TidyCoin();


    // 是否需要进入游戏服务器
    bool    CheckEnterToGameSvr();

protected:
    stRobotCfg      m_robotCfg;
    CCooling        m_robotCool;
    uint32          m_loginOutTime;
    int32           m_leaveRound;
    bool            m_needLoginOut;

};
















#endif //SERVER_ROBOT_H
