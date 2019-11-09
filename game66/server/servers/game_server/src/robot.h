//
// Created by toney on 16/5/18.
//

#ifndef SERVER_ROBOT_H
#define SERVER_ROBOT_H

#include "game_player.h"
#include "svrlib.h"

class CGameRobot : public CGamePlayer
{
public:
    CGameRobot();
    ~CGameRobot();
    virtual void  	OnLogin();
    virtual void 	OnGameEnd();

    virtual void 	OnTimeTick(uint64 uTime,bool bNewDay);
    // 是否需要回收
    virtual bool 	NeedRecover();
public:
    bool    NeedBackPool();
    bool    NeedBackLobby();
    // 是否破产
    bool    IsBankrupt();

    // 回归队列
    void    BackPool();
    void    SetLeaveTime(uint32 leaveTime){ m_leaveTime = leaveTime; }

protected:
    int64       m_loginInScore;
    int64       m_loginInCoin;
    uint32      m_leaveTime;
    int32       m_leaveRound;       // 剩余离开局数
    CCooling    m_coolBlocker;      // 清除黑名单

};















































#endif //SERVER_ROBOT_H
