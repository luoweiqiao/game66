//
// Created by toney on 16/4/14.
//

#include "mission_mgr.h"
#include "stdafx.h"
#include "db_struct_define.h"

using namespace std;
using namespace svrlib;

CMissionMgr::CMissionMgr()
{
    m_pHost = NULL;
}
CMissionMgr::~CMissionMgr()
{
    m_pHost = NULL;
}
void  CMissionMgr::AttachPlayer(CPlayer* pPlayer)
{
    m_pHost = pPlayer;
}
CPlayer* CMissionMgr::GetAttachPlayer()
{
    return m_pHost;
}
void	CMissionMgr::SetMission(map<uint32,stUserMission> mission)
{
    m_mission = mission;
    m_pHost->SetLoadState(emACCDATA_TYPE_MISS,1);
    GetMission();

	map<uint32, stUserMission>::iterator it_p = m_mission.begin();
	for (; it_p != m_mission.end(); ++it_p)
	{
		stUserMission& refMiss = it_p->second;

		if (m_pHost != NULL && m_pHost->IsRobot() == false)
		{
			LOG_DEBUG("uid:%d,size:%d,msid:%d, rtimes:%d, ctimes:%d", m_pHost->GetUID(), m_mission.size(), refMiss.msid, refMiss.rtimes, refMiss.ctimes);

		}
	}
}

void    CMissionMgr::SaveMiss()
{
    if(m_pHost->IsRobot())
        return;
        
    CDBMysqlMgr::Instance().SaveUserMission(m_pHost->GetUID(),m_mission);
    vector<uint32> dels;
    map<uint32,stUserMission>::iterator it = m_mission.begin();
    for(;it != m_mission.end();++it)
    {
        stUserMission& refMiss = it->second;
        switch(refMiss.update)
        {
        case emDB_ACTION_UPDATE:
        case emDB_ACTION_INSERT:
            {
                refMiss.update = emDB_ACTION_NONE;
            }break;
        case emDB_ACTION_DELETE:
            {
                dels.push_back(refMiss.msid);
            }break;
        default:
            break;
        }
    }
    for(uint32 i=0;i<dels.size();++i)
    {
        RemoveMission(dels[i]);
    }
}
//获得用户的任务
void	CMissionMgr::GetMission()
{
    if(m_pHost->IsRobot())
        return;
    
    map<uint32,stMissionCfg> missioncfg = CDataCfgMgr::Instance().GetAllMissionCfg();

	if (m_pHost != NULL && m_pHost->IsRobot() == false)
	{
		LOG_DEBUG("all mis - uid:%d,size:%d", m_pHost->GetUID(), missioncfg.size());

	}


    map<uint32,stMissionCfg>::iterator it = missioncfg.begin();
    for(;it != missioncfg.end();++it)
    {
        if(it->second.status == 1){
            continue;
        }
        if(IsExistMission(it->first))
            continue;
        AddMission(it->first);
    }
    // 删除不存在的任务
    map<uint32,stUserMission>::iterator it1 = m_mission.begin();
    for(;it1 != m_mission.end();++it1)
    {
        stUserMission& refMiss = it1->second;
        const stMissionCfg* pCfg = CDataCfgMgr::Instance().GetMissionCfg(refMiss.msid);
        if( pCfg == NULL || pCfg->status == 1){
            refMiss.update = emDB_ACTION_DELETE;
        }
    }
    SaveMiss();

	//uint32 msid;    // ID
	//uint32 rtimes;  // 任务进度
	//uint32 ctimes;  // 任务完成次数
	//uint32 ptime;   // 操作时间
	//uint8  update;  // 更新数据库类型
	//uint32 cptime;  // 完成时间
}
//操作用户任务
void 	CMissionMgr::ActMission(uint16 type, uint32 value, uint32 cate1, uint32 cate2, uint32 cate3, uint32 cate4)
{
    if(m_pHost->IsRobot())
        return;
    
	LOG_DEBUG("uid:%d,cate1:%d, cate2:%d, cate3:%d, cate4:%d,value:%d, type:%d, m_mission_size:%d",
		m_pHost->GetUID(), cate1, cate2, cate3, cate4, value, type, m_mission.size());

    MAP_MISSION::iterator it = m_mission.begin();
    for(;it != m_mission.end();++it)
    {
        //已完成的任务不再累加
        stUserMission& refUserMiss = it->second;
        const stMissionCfg* pMissCfg = CDataCfgMgr::Instance().GetMissionCfg(refUserMiss.msid);
        if(pMissCfg == NULL)
            continue;

        //if(pMissCfg->mtimes <= refUserMiss.rtimes){
        //    continue;
        //}


		bool bflag = false;
		uint32 maxCount = pMissCfg->mtimes.size();
		if (maxCount > 0)
		{
			if (pMissCfg->mtimes[maxCount - 1] <= refUserMiss.rtimes)
			{
				bflag = true;
			}
		}
		string strmtimes;
		for (unsigned int i = 0; i < pMissCfg->mtimes.size(); ++i)
		{
			strmtimes += CStringUtility::FormatToString("%d ", pMissCfg->mtimes[i]);
		}
		//LOG_ERROR("actoin mission - uid:%d,msid:%d,maxCount:%d,cycletimes:%d,ctimes:%d,rtimes:%d,bflag:%d,strmtimes:%s",
		//	m_pHost->GetUID(), refUserMiss.msid, maxCount, pMissCfg->cycletimes, refUserMiss.ctimes, refUserMiss.rtimes, bflag, strmtimes.c_str());
		//达到最大就不再累加
		if (bflag) {
			continue;
		}

		// 完成次数大于循环次数的时候跳过
        if(pMissCfg->cycletimes <= refUserMiss.ctimes){
            continue;
        }

        //判断动作类型
        if (pMissCfg->type != type){
            continue;
        }
        if (pMissCfg->cate1 > 0 && pMissCfg->cate1 != cate1){
            continue;
        }
        if (pMissCfg->cate2 > 0 && pMissCfg->cate2 != cate2){
            continue;
        }
        if (pMissCfg->cate3 > 0 && pMissCfg->cate3 != cate3){
            continue;
        }
        if (pMissCfg->cate4 > 0 && pMissCfg->cate4 != cate4){
            continue;
        }
		//uint32 old_rtimes = refUserMiss.rtimes;
        if(pMissCfg->straight == 1 && value == 0)
		{// 连续任务
            refUserMiss.rtimes = 0;
        }
		else
		{
			if (value == 0)
			{
				continue;
			}
			bool bIsSingJetton = (type == net::MISSION_TYPE_PLAY_JETTON || type == net::MISSION_TYPE_PLAY_SING_WIN);
			if (bIsSingJetton && pMissCfg->mtimes.size() > 0)
			{
				unsigned int index = 0;
				for (; index < pMissCfg->mtimes.size(); ++index)
				{
					if (pMissCfg->mtimes[index] >= refUserMiss.rtimes)
					{
						break;
					}
				}
				if (value >= pMissCfg->mtimes[index])
				{
					refUserMiss.rtimes = value;
				}
			}
			else
			{
				refUserMiss.rtimes += value;
			}
        }
        refUserMiss.ptime = getSysTime();// 操作时间
        if(refUserMiss.update == emDB_ACTION_NONE)
		{
            refUserMiss.update = emDB_ACTION_UPDATE;
        }
        //判断是否自动领取
        if(pMissCfg->autoprize == 1 && IsFinished(&refUserMiss))
		{
            RewardMission(&refUserMiss);
        }
		else
		{
            SendMission2Client(refUserMiss);
        }
    }
}

//获得任务奖励
void CMissionMgr::GetMissionPrize(uint32 msid)
{
    stUserMission* pMission = GetMission(msid);
    if(pMission == NULL)
	{
		if (m_pHost != NULL && m_pHost->IsRobot() == false)
		{
			LOG_ERROR("CPlayer::getMissionPrize the uid = %d,the msid = %d,has no mission", m_pHost->GetUID(), msid);

		}
        return ;
    }
    //判断不能领
    if (IsFinished(pMission) == false)
        return;
    RewardMission(pMission);
}
// 添加任务
bool    CMissionMgr::AddMission(uint32 msid)
{
    const stMissionCfg* pCfg = CDataCfgMgr::Instance().GetMissionCfg(msid);
    if (pCfg == NULL || pCfg->status == 1)
        return false;
    if (IsExistMission(msid))
        return false;

    stUserMission mission;
    mission.msid = msid;
    mission.rtimes = 0;
    mission.ptime  = 0;
    mission.ctimes = 0;
    mission.cptime = 0;
    mission.update = emDB_ACTION_INSERT;
    m_mission.insert(make_pair(msid,mission));

    return true;
}
bool    CMissionMgr::RemoveMission(uint32 msid)
{
    m_mission.erase(msid);
    return true;
}
// 是否有这个任务
bool    CMissionMgr::IsExistMission(uint32 msid)
{
    MAP_MISSION::iterator it = m_mission.find(msid);
    if(it == m_mission.end()){
        return false;
    }
    return true;
}
stUserMission* CMissionMgr::GetMission(uint32 msid)
{
    MAP_MISSION::iterator it = m_mission.find(msid);
    if(it == m_mission.end()){
        return NULL;
    }
    return &it->second;
}
// 重置任务
void    CMissionMgr::ResetMission(uint8 cycle)
{
    //先判断用户的任务
    MAP_MISSION::iterator it = m_mission.begin();
    for(;it != m_mission.end();++it)
    {
        stUserMission& task = it->second;
        const stMissionCfg* pCfg = CDataCfgMgr::Instance().GetMissionCfg(task.msid);
        if(pCfg == NULL){
            continue;
        }
        //循环任务
        if(task.ptime > 0 && pCfg->cycle == cycle)
        {
            task.rtimes = 0;
            task.ptime = getSysTime();
            task.ctimes = 0;
            task.update = emDB_ACTION_UPDATE;
            task.cptime = 0;
        }
    }
}
// 能否接受任务
bool    CMissionMgr::CanAcceptMiss(stMissionCfg* pCfg)
{
    if(IsExistMission(pCfg->msid)){
        return false;
    }
    // 其它条件判断


    return true;
}
// 任务是否完成
bool    CMissionMgr::IsFinished(stUserMission* pMission)
{
	return false;

    const stMissionCfg* pMissCfg = CDataCfgMgr::Instance().GetMissionCfg(pMission->msid);
    if(pMissCfg == NULL)
        return false;

    //判断不能领
    //if(pMission->rtimes < pMissCfg->mtimes){
    //    //LOG_ERROR("CPlayer::getMissionPrize the uid = %d,the msid = %d,the rtimes = %d,the mtimes = %d",m_pHost->GetUID(),pMission->msid,pMission->rtimes,pMissCfg->mtimes);
    //    return false;
    //}
	bool bIsEquivalent = false;
	if (pMissCfg->mtimes.size() > 0)
	{
		for (uint32 i = 0; i < pMissCfg->mtimes.size(); i++)
		{
			if (pMission->rtimes == pMissCfg->mtimes[i])
			{
				if (m_pHost != NULL && m_pHost->IsRobot() == false)
				{
					LOG_ERROR("CPlayer::getMissionPrize the - i:%d,uid = %d,the msid = %d,the rtimes = %d,the mtimes = %d", i, m_pHost->GetUID(), pMission->msid, pMission->rtimes, pMissCfg->mtimes[i]);

				}
				bIsEquivalent = true;
				break;
				//return false;
			}
			//else
			//{
			//	LOG_ERROR("CPlayer::getMissionPrize the - i:%d,uid = %d,the msid = %d,the rtimes = %d,the mtimes = %d",i, m_pHost->GetUID(), pMission->msid, pMission->rtimes, pMissCfg->mtimes[i]);

			//	break;
			//}
		}
	}
	else
	{
		if (m_pHost != NULL && m_pHost->IsRobot() == false)
		{
			LOG_ERROR("CPlayer::getMissionPrize the - size:%d,uid = %d,the msid = %d,the rtimes = %d", pMissCfg->mtimes.size(), m_pHost->GetUID(), pMission->msid, pMission->rtimes);

		}

		return false;
	}
	if (bIsEquivalent == false)
	{
		return false;
	}

    if(pMission->ctimes >= pMissCfg->cycletimes)
	{
		if (m_pHost != NULL && m_pHost->IsRobot() == false)
		{
			LOG_ERROR("CPlayer::getMissionPrize the uid = %d,the msid = %d,the ctimes = %d,the cycletimes = %d", m_pHost->GetUID(), pMission->msid, pMission->ctimes, pMissCfg->cycletimes);

		}
        return false;
    }

    return true;
}
// 奖励任务
bool    CMissionMgr::RewardMission(stUserMission* pMission)
{
    const stMissionCfg* pMissCfg = CDataCfgMgr::Instance().GetMissionCfg(pMission->msid);
    if(pMissCfg == NULL)
        return false;
    pMission->rtimes = 0;
    pMission->ctimes++;
    pMission->cptime = getSysTime();// 完成时间
    if(pMission->update == emDB_ACTION_NONE) {
        pMission->update = emDB_ACTION_UPDATE;
    }
    int64 diamond, coin, ingot, score, cvalue;
    diamond = coin = ingot = score = cvalue = 0;
    for(uint32 i=0;i<pMissCfg->missionprize.size();++i)
    {
        uint32 qty = pMissCfg->missionprize[i].qty;
        switch (pMissCfg->missionprize[i].poid)
        {
        case emACC_VALUE_DIAMOND:
            {
                diamond = qty;
            }break;
        case emACC_VALUE_COIN:
            {
                coin = qty;
            }break;
        case emACC_VALUE_INGOT:
            {
                ingot = qty;
            }break;
        case emACC_VALUE_SCORE:
            {
                score = qty;
            }break;
        case emACC_VALUE_CVALUE:
            {
                cvalue = qty;
            }break;
        default:
            break;
        }
    }
    m_pHost->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_TASK,pMission->msid,diamond,coin,ingot,score,cvalue,0);
    SendMission2Client(*pMission);
    m_pHost->UpdateAccValue2Client();
    return true;
}
bool	CMissionMgr::SendMissionData2Client()
{
    net::msg_send_all_mission_rep msg;
    map<uint32 ,stUserMission>::iterator it = m_mission.begin();
    for(;it != m_mission.end();++it)
    {
        net::mission_data* pInfo = msg.add_missions();
        pInfo->set_msid(it->second.msid);
        pInfo->set_ctimes(it->second.ctimes);
        pInfo->set_rtimes(it->second.rtimes);
        pInfo->set_cptime(it->second.cptime);

		if (m_pHost != NULL && m_pHost->IsRobot() == false)
		{
			LOG_DEBUG("CPlayer::all the uid = %d,the msid = %d,the ctimes = %d,the rtimes = %d",
				m_pHost->GetUID(), it->second.msid, it->second.ctimes, it->second.rtimes);
		}
    }
    m_pHost->SendMsgToClient(&msg,net::S2C_MSG_SEND_ALL_MISSION_REP);
    return true;
}

bool	CMissionMgr::SendMission2Client(stUserMission& mission)
{
    net::msg_send_mission_rep msg;
    net::mission_data* pInfo = msg.mutable_mission();
    pInfo->set_msid(mission.msid);
    pInfo->set_ctimes(mission.ctimes);
    pInfo->set_rtimes(mission.rtimes);
    pInfo->set_cptime(mission.cptime);
    m_pHost->SendMsgToClient(&msg,net::S2C_MSG_SEND_MISSION_REP);

	if (m_pHost != NULL && m_pHost->IsRobot() == false)
	{
		LOG_DEBUG("CPlayer::all the uid = %d,the msid = %d,the ctimes = %d,the rtimes = %d",
			m_pHost->GetUID(), mission.msid, mission.ctimes, mission.rtimes);

	}

    return true;
}









