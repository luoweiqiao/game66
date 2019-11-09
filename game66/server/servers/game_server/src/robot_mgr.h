//
// Created by toney on 16/5/18.
//

#ifndef SERVER_ROBOT_MGR_H
#define SERVER_ROBOT_MGR_H

#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "robot.h"

class CGamePlayer;
class CGameTable;

class CRobotMgr : public AutoDeleteSingleton<CRobotMgr>
{
public:
    CRobotMgr();
    ~CRobotMgr();

    bool	Init();
    void	ShutDown();
    void	OnTimeTick();

    CGameRobot* GetRobot(uint32 uid);
    // 请求分配一个机器人
    bool    RequestOneRobot(CGameTable* pTable, CGamePlayer* pPlayer);

	bool    RequestXRobot(int count,CGameTable* pTable);

    bool    AddRobot(CGameRobot* pRobot);
    bool    RemoveRobot(CGameRobot* pRobot);

    // 空闲机器人数量
    uint32  GetFreeRobotNum();
    void    GetFreeRobot(vector<CGameRobot*>& robots);

    // 派发机器人占桌
    void    DispatchRobotToTable();

protected:
    void    CheckRobotBackPool();
    void    CheckRobotBackLobby();


protected:
    typedef 	stl_hash_map<uint32, CGameRobot*>    MAP_ROBOTS;
    MAP_ROBOTS  m_mpRobots;
    CCooling    m_DispatchCool;
    bool        m_isOpenRobot;

};
















































#endif //SERVER_ROBOT_MGR_H
