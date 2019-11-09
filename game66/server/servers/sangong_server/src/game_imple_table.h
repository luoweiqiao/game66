//
// Created by toney on 16/4/6.
//
// 牛牛的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "poker/sangong_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_sangong;

class CGamePlayer;
class CGameRoom;

// 三公游戏桌子
class CGameSanGongTable : public CGameTable
{
public:
    CGameSanGongTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameSanGongTable();

// 重载基类函数
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);

public:
    //配置桌子
    virtual bool Init();
    virtual void ShutDown();

    //复位桌子
    virtual void ResetTable();
    virtual void OnTimeTick();
    //游戏消息
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
 
public:
    // 游戏开始
    virtual bool OnGameStart();
    // 游戏结束
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //用户同意
    virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //玩家进入或离开
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);

public:
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
    void    CalcPlayerInfo(uint16 chairID,int64 winScore);
    // 重置游戏数据
    void    ResetGameData();
    // 做牌发牌
	void	DispatchCard();
protected:
    // 写入出牌log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int64 score);
    // 写入庄家位log
    void    WriteBankerLog(uint16 chairID);

    // 是否能够开始游戏
    bool    IsCanStartGame();
    // 检测筹码是否正确
    bool    CheckJetton(uint16 chairID,int64 score);
    // 获得机器人的铁壁
    int64   GetRobotJetton(uint16 chairID);

    uint16  GetPlayNum();

    void    InitBanker(bool isApply);
    void    SendCardToClient();

    // 结算分数
    void    CalculateScore();

    // 检测提前结束
    void    CheckOverTime();

    //游戏事件
protected:
	//游戏状态
	bool    IsUserPlaying(WORD wChairID);
    
	//加注事件
    bool    OnUserAddScore(WORD wChairID, int64 lScore);
    //申请庄家
    bool    OnUserApplyBanker(WORD wChairID,int32 score);
    //摆牌
    bool    OnUserChangeCard(WORD wChairID);
    //发送摆牌
    void    SendChangeCard(WORD wChairID,CGamePlayer* pPlayer);
    int32   GetChangeCardNum();

    //机器人操作
protected:
    bool    OnRobotOper();
    bool    OnRobotApplyBanker();
    bool    OnRobotReady();
    bool    OnRobotChangeCard();

    void    CheckAddRobot();
    void    CheckRobotLeave();
    void    SetRobotThinkTime();


	//游戏变量
protected:
	WORD							m_wBankerUser;							//庄家用户
    bool                            m_isNeedBanker;                         //是否需要庄家用户

	//用户状态
protected:
	BYTE							m_cbPlayStatus[GAME_PLAYER];			//游戏状态
    BYTE                            m_szApplyBanker[GAME_PLAYER];           //申请庄家状态
    BYTE                            m_szShowCardState[GAME_PLAYER];         //摆牌状态

	//扑克变量
protected:
	BYTE							m_cbHandCardData[GAME_PLAYER][MAX_COUNT];       //桌面扑克


	//下注信息
protected:
    int64						    m_lTableScore[GAME_PLAYER];				//下注数目
    BYTE                            m_cbTableCardType[GAME_PLAYER];         //桌面牌型
    BYTE                            m_winMultiple[GAME_PLAYER];             //输赢倍数
    int64                           m_lWinScore[GAME_PLAYER];               //输赢分数
    int64                           m_lTurnMaxScore[GAME_PLAYER];           //最大下注

protected:
    CSangongLogic                   m_gameLogic;                            //游戏逻辑
    CCooling                        m_coolRobot;                            //机器人CD

};

#endif //SERVER_GAME_IMPLE_TABLE_H

