//
// Created by toney on 16/4/14.
//
#pragma once

#include <map>
#include "svrlib.h"
#include "db_struct_define.h"

using namespace std;
using namespace svrlib;

class CPlayer;

class CMissionMgr
{
public:
    CMissionMgr();
    ~CMissionMgr();

    void  AttachPlayer(CPlayer* pPlayer);
    CPlayer* GetAttachPlayer();

    void	SetMission(map<uint32,stUserMission> mission);
    void    SaveMiss();
    //获得用户的任务
    void	GetMission();

    //操作用户的任务
    void	ActMission(uint16 type,uint32 value = 1,uint32 cate1 = 0,uint32 cate2 = 0,uint32 cate3 = 0,uint32 cate4 = 0);
    //获得任务奖励
    void 	GetMissionPrize(uint32 msid);

    // 添加任务
    bool    AddMission(uint32 msid);
    bool    RemoveMission(uint32 msid);

    // 是否有这个任务
    bool    IsExistMission(uint32 msid);
    stUserMission* GetMission(uint32 msid);
    // 重置任务
    void    ResetMission(uint8 cycle);


    // 能否接受任务
    bool    CanAcceptMiss(stMissionCfg* pCfg);
    // 任务是否完成
    bool    IsFinished(stUserMission* pMission);
    // 奖励任务
    bool    RewardMission(stUserMission* pMission);

    bool	SendMissionData2Client();
    bool	SendMission2Client(stUserMission& mission);


protected:
    typedef map<uint32,stUserMission> MAP_MISSION;
    MAP_MISSION     m_mission;
    CPlayer*        m_pHost;


};
