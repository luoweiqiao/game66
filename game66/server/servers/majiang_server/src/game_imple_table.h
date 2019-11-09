//
// Created by toney on 16/4/6.
//
// 麻将的桌子逻辑

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "majiang_logic.h"
#include "majiang_config.h"

using namespace svrlib;
using namespace std;
using namespace game_majiang;

class CGamePlayer;
class CGameRoom;

//效验类型
enum enEstimatKind
{
    EstimatKind_OutCard,			//出牌效验
    EstimatKind_GangCard,			//杠牌效验
    EstimatKind_Tail,               //海底校验
    EstimatKind_GangTing,           //杠听操作
	EstimatKind_QiangGangHu,        //抢杠操作

};
enum emGangTingState
{
    emGANG_TING_NULL,       //空状态
    emGANG_TING_SELF_HU,    //杠听自己胡
    emGANG_TING_OTHER_HU,   //杠听他人胡
    emGANG_TING_QIANG,      //抢杠胡
};


enum em_CARD_TYPE_PRO_INDEX
{
	Pro_Index_FiveDuiZi = 0,
	Pro_Index_FourDuiZi,
	Pro_Index_ThreeKeZi,
	Pro_Index_TwoKeZi_OneDuiZi,
	Pro_Index_TwoKeZi,
	Pro_Index_TwelveTongHua,
	Pro_Index_ElevenTongHua,
	Pro_Index_TenTongHua,
	Pro_Index_RandSingle,
	Pro_Index_MAX,
};


// 麻将游戏桌子
class CGameMaJiangTable : public CGameTable
{
public:
    CGameMaJiangTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameMaJiangTable();

    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

    virtual void GetTableFaceInfo(net::table_face_info* pInfo);

public:
    //配置桌子
    virtual bool Init();
    virtual void ShutDown();
	virtual	bool ReAnalysisParam();
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
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
	virtual bool MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard);

protected:
	// 获取单个下注的是机器人还是玩家  add by har
    // 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);

    bool    IsCanStart();
    // 重置游戏数据
    void    ResetGameData();
    // 流局结束
    void    GameOverNoWin();
    // 杠听后无人操作继续流程
    void    GangTingNoHuEnd(uint16 chairID);
    // 重置用户操作信息
    void    ResetUserOperInfo();

	void    AddBlockers();

	int		GetProCardType();

	bool    SetControlCardData();

	bool	GetHandCard_FiveDuiZi(uint16 chairID);
	bool	GetHandCard_FourDuiZi(uint16 chairID);
	bool	GetHandCard_ThreeKeZi(uint16 chairID);
	bool	GetHandCard_TwoKeZi_OneDuiZi(uint16 chairID);
	bool	GetHandCard_TwoKeZi(uint16 chairID);
	bool	GetHandCard_TwelveTongHua(uint16 chairID);
	bool	GetHandCard_ElevenTongHua(uint16 chairID);
	bool	GetHandCard_TenTongHua(uint16 chairID);

	// isNeedTingCard : 是否需要处于听牌状态 add by har
	bool    SetRobotTingCardZiMo(uint16 chairID, BYTE &cbOutSendCardData, bool isNeedTingCard = true);

	bool	StartSendHandCard();
	bool	StartChangeHandCard();

public:
    //获得摸牌数
    BYTE    GetSendCardCount(){ return m_cbSendCardCount; }
    //获得已经出牌数量(胡绝张)
    uint32  GetShowCardNum(BYTE card);
    //获得牌池牌数量
    uint32  GetCardPoolSize();

	WORD	GetProvideUser() { return m_wProvideUser; }
	WORD	GetPlayerCount() { return m_wPlayerCount; }
	bool    GetOperHuCardIsListen() {
		if (m_wOperHuCardPlayer < m_wPlayerCount) { return m_cbListenStatus[m_wOperHuCardPlayer] == TRUE; }
		return false;
	}
	bool    GetOperHuCardIsGangFalwore() {
		if (m_wOperHuCardPlayer < m_wPlayerCount) { return m_bArrGangFlowerStatus[m_wOperHuCardPlayer]; }
		return false;
	}
	uint32  GetHuCardPlayerID() { return GetPlayerID(m_wOperHuCardPlayer); }
	bool	GetOperPlayerHuCardType(BYTE cbCard)
	{
		if (m_wOperHuCardPlayer < m_wPlayerCount )
		{
			if (m_vecArrUserHuCard[m_wOperHuCardPlayer].size() == 1 && cbCard == m_vecArrUserHuCard[m_wOperHuCardPlayer][0])
			{
				return true;
			}
		}
		return false;
	}

protected:
    // 用户出牌
	bool    OnUserOutCard(uint16 chairID, BYTE cardData, bool bListen = false);
    // 用户操作
    bool    OnUserOperCard(uint16 chairID,uint32 operCode,BYTE operCard);
	// 用户操作
	bool    OnUserOperTrustee(uint16 chairID, uint32 trustee);

	bool    OnUserNotifyHu(uint16 chairID);

	bool    OnUserOutCardNotifyHu(uint16 chairID);


    // 用户要海底操作
    bool    OnUserNeedTailOper(uint16 chairID,uint32 operCode);
    // 用户起手胡操作
    bool    OnUserOpenHuOper(uint16 chairID,uint32 operCode);

    // 胡牌操作处理
    bool    ChiHuOper(uint16 chairID,bool bAction);
    // 杠牌操作处理
    bool    GangPaiOper(uint16 chairID,BYTE cbCard,bool bPublic);
	//更新可以胡牌数量
	//uint32  UpdateNotifyHuCard(uint8 chairID, BYTE cbCard);

	bool  UpdateNotifyHuCardCount(BYTE cbCard, BYTE cbCount);

	bool  UpdateNotifyHuCardCount(uint16 chairID,BYTE cbCard, BYTE cbCount);
	// 发送胡牌提示
	//bool	SendTingNotifyHuCard(uint16 chairID);
    // 发送操作
    bool    SendOperNotify();
    // 通知听牌
    bool    NotifyListen(uint16 chairID);
    // 刷新牌面数据给前端
    bool    FlushDeskCardToClient();
	bool    FlushDeskCardToClient(uint16 wChairID);
    // 刷新手牌数据给前端
    bool    FlushHandCardToClient(uint16 chairID);

    // 显示公共牌
    bool    ShowPublicCards(uint16 chairID,uint32 showType,const vector<BYTE>& cards);
    // 派发扑克
    bool    DispatchCardData(uint16 curUser,bool bNotGang=true);
    // 响应判断
    bool    EstimateUserRespond(WORD wCenterUser, BYTE cbCenterCard, enEstimatKind EstimatKind);
    // 海底操作处理
    bool    EstimateUserTailRespond();
    // 要海底后操作
    bool    EstimateNeedTailOper(uint16 chairID);
    // 杠听后操作
    bool    EstimateGangTing(uint16 chairID);
    bool    EstimateGangTingQiang(uint16 chairID);

    // 计算玩家胡牌结果
    void    CalcPlayerHuResult(uint16 chairID,uint32 dwChiHuRight,bool bAction);

    // 检查是否胡海底
    bool    CheckHuTail();
    // 计算杠听胡(胡最大的牌)
    bool    CheckGangTingHu(uint16 chairID);

    // 辅助函数
protected:
    uint16  GetNextUser(uint16 chairID);
    void    ResetCoolTime();
    void    SwitchUserAction(uint16 chairID,BYTE tarCard,vector<net::mjaction>& actions);

    //获得牌数
    BYTE    GetCardCount(uint16 chairID,BYTE card);
    //选择一张出牌
    BYTE    GetAIOutCard(uint16 chairID,int & iOutIndex);
    //test 随机换位
    void    RandChangeSeat(uint16 chairID);

protected:
    void    CheckAddRobot();
    //机器人出牌
    bool    OnRobotOutCard();
    //超时自动放弃
    bool    OnTimeOutOper();
    //听牌自动出牌
    bool    OnListenOutCard();
	//托管自动出牌
	bool    OnTrusteeOutCard();
	//托管操作
	bool    OnTrusteeOperCard();

    //初始化洗牌
    void    InitRandCard();
    //摸一张牌
    BYTE    PopCardFromPool();
    //处理起手胡
    bool    ProcessOpeningHu();
    //检查起手动作
    void    CheckStartAction(bool bInit);

	bool	CheckUserTingCard(uint32 chairID);

	bool    OnUserRecordTingCard(uint16 chairID, vector<BYTE> tingCards);

	bool	CheckUserTingCardIsChange(uint16 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount);

    //计算番数
    int32   CalcBaseFanCount(uint16 chairID,stChiHuResult huResult);
    //计算扎鸟
    bool    CalcHitBird(uint32 provideID,uint32 winID,bool isMulPao);
    //计算杠的分数
    bool    CalcGangScore();
    //计算胡牌分数
    bool    CalcHuScore();
    bool    CalcGuoBiaoHuScore();

    //设置下轮庄家
    bool    SetNextBanker();
    //获得所有胡牌玩家
    void    GetAllHuUser(vector<uint16>& huUsers);
    //检查手牌是否有赖子
    uint8   GetKingCardCount(uint16 chairID);
    //抓鸟点数
    uint8   GetBirdValue(uint8 card);

    //设置赢分
    void    SetWinScore(uint16 winID,uint16 loseID,int64 score);

	//log
	void    WriteOutCardLog(uint16 chairID, uint32 opercode, uint8 cardData[], uint8 cardCount);

	void    WriteUserHandCard(uint32 code);

	void    WriteGameEndLog(uint16 chairID, uint32 opercode, vector<BYTE> actions);
	void    WriteGameEndLog(uint16 chairID, uint16 fangpao, uint8 cardData, int64 score);

    // 获取当前局的活跃福利控制玩家列表---每一局开始时调用
    bool    SetActiveWelfareCtrlList();

    // 判断当前玩家是否在活跃福利控制列表中
    bool    GetInActiveWelfareCtrlList(uint32 uid, uint64 &aw_max_win);

    // 设置活跃福利玩家听牌自摸
    bool    SetAWTingCardZiMo(uint16 chairID, BYTE &cbOutSendCardData, uint64 aw_max_win);

	// 设置新注册玩家福利的听牌自摸
	bool    SetNRWTingCardZiMo(uint16 chairID, BYTE &cbOutSendCardData);
	// 设置新注册玩家福利的机器人听牌自摸
	bool    SetNRWRobotTingCardZiMo(uint16 chairID, BYTE &cbOutSendCardData);
	// 新注册玩家福利：根据配置的牌型概率获取相应的牌型---必赢局
	int		GetNRWProCardType();
	// 设置新注册玩家福利：必赢局的牌型---设置牌型
	bool    SetNRWControlCardData();
	
	//幸运值控制---听牌自摸
	bool    SetLuckyTingCardZiMo(uint16 chairID, BYTE &cbOutSendCardData);
	//获取幸运值控制玩家位置
	void	GetLuckyPlayerUid();

	// 设置库存输赢  add by har
    // return  true 触发库存输赢  false 未触发
	bool SetStockWinLose();
	// 设置库存玩家听牌自摸发牌  add by har
	// return : true 设置成功  false 设置失败
	bool SetStockTingCardZiMo(uint16 chairID, uint8 &cbOutSendCardData);

	void	CheckPlayerScoreManyLeave();

    //控制变量
protected:
	int								m_iArrDispatchCardPro[Pro_Index_MAX];	//发牌型概率
	uint32							m_uRobotZiMoPro;
    //游戏变量
protected:
    uint8							m_siceCount[2];						    //骰子点数
    WORD							m_wBankerUser;							//庄家用户
    WORD                            m_wNextBankerUser;                      //下个庄家
public:
    BYTE							m_cbCardIndex[GAME_PLAYER][MAX_INDEX];	//用户扑克
protected:
	bool							m_bTrustee[GAME_PLAYER];
	uint32							m_uTimeOutCount[GAME_PLAYER];
protected:
    //临时变量
    int64                           m_lGameScore[GAME_PLAYER];              //游戏得分
    int64                           m_lEachScore[GAME_PLAYER][GAME_PLAYER]; //独立结算分数
	stTempWeaveItem				    m_TempWeaveItem;						//组合扑克

    //出牌信息
protected:
    WORD							m_wOutCardUser;							//出牌用户
    BYTE							m_cbOutCardData;						//出牌扑克
    BYTE							m_cbOutCardCount;						//出牌数目
    BYTE							m_cbDiscardCount[GAME_PLAYER];			//丢弃数目
    BYTE							m_cbDiscardCard[GAME_PLAYER][55];		//丢弃记录
    BYTE							m_cbGangCount;							//杠牌次数
    BYTE                            m_cbMustLeft;                           //保留牌数
	BYTE							m_cbArrOutCardCount[GAME_PLAYER];		//出牌数目
	BYTE							m_cbArrOperIndex[GAME_PLAYER];			//操作下标
	uint32							m_cbPlayerOperIndex;					//操作下标

    //发牌信息
protected:
    BYTE							m_cbSendCardData;						//发牌扑克
    BYTE							m_cbSendCardCount;						//发牌数目
	WORD							m_wSendCardUser;						//发牌用户
	WORD							m_wLastOutCardUser;						//出牌用户

public:
    list<BYTE>                      m_poolCards;                            //牌池扑克

    //运行变量
protected:
    WORD							m_wResumeUser;							//还原用户
    WORD							m_wCurrentUser;							//当前用户
    WORD							m_wProvideUser;							//供应用户
    BYTE							m_cbProvideCard;						//供应扑克

    //状态变量
protected:
    bool							m_bSendStatus;							//发牌状态
    bool							m_bGangStatus;							//抢杆状态
    bool							m_bEnjoinChiHu[GAME_PLAYER];			//禁止吃胡
    bool							m_bEnjoinChiPeng[GAME_PLAYER];			//禁止吃碰

    //用户状态
public:
    bool							m_bResponse[GAME_PLAYER];				//响应标志
    DWORD							m_dwUserAction[GAME_PLAYER];			//用户动作

    BYTE							m_cbOperateCard[GAME_PLAYER];			//操作扑克
    DWORD							m_dwPerformAction[GAME_PLAYER];			//执行动作
    BYTE							m_cbListenStatus[GAME_PLAYER];			//听牌状态
	bool							m_bArrTianTingStatus[GAME_PLAYER];		//天听状态
	bool							m_bArrTingAfterMingGang[GAME_PLAYER];	//听后是否进行明杠
	WORD							m_wOperHuCardPlayer;					//操作胡牌
	vector<BYTE>					m_vecArrUserTingCard[GAME_PLAYER];		//用户听牌
	vector<BYTE>					m_vecArrUserHuCard[GAME_PLAYER];		//用户胡牌
	map<BYTE, vector<tagAnalyseTingNotifyHu>> m_mpTingNotifyHu[GAME_PLAYER];//用户听牌提示胡
	vector<tagAnalyseTingNotifyHu>  m_vecTingNotifyHu[GAME_PLAYER];//用户听牌提示胡
	void							AddUserPassHuCount(uint16 chairID);
	int								m_iUserPassHuCount[GAME_PLAYER];
	int								m_iUserPassHuFlag[GAME_PLAYER];
	bool							m_bArrGangFlowerStatus[GAME_PLAYER];	// 杠上开花状态

    //组合扑克
protected:
    BYTE							m_cbWeaveItemCount[GAME_PLAYER];		        //组合数目
    stWeaveItem					    m_WeaveItemArray[GAME_PLAYER][MAX_WEAVE];		//组合扑克

    //结束信息
protected:
    BYTE							m_cbChiHuCard;							//吃胡扑克
    WORD                            m_fangPaoUser;                          //放炮玩家
    stChiHuResult				    m_ChiHuResult[GAME_PLAYER];				//吃胡结果
    stActionOper                    m_openingHu[GAME_PLAYER];               //起手胡类型

    BYTE                            m_addHitBirdCount[GAME_PLAYER];         //附加中鸟数
    BYTE                            m_hitBirdCount[GAME_PLAYER];            //中鸟数
    BYTE                            m_birdCards[GAME_PLAYER][6];            //鸟牌
    vector<BYTE>                    m_allBirdCards;                         //全部鸟牌

    BYTE                            m_tailCard;                             //海底牌
    BYTE                            m_tailCardOper[GAME_PLAYER];            //海底牌操作

    BYTE                            m_gangTingState;                        //杠听状态(1杠上开花,2杠上炮,3抢杠胡)
    vector<BYTE>                    m_gangTingCards;                        //杠听翻牌

	bool							m_NRWRobotZiMo;							//新帐号福利功能：机器人是否必须自摸
	int								m_NRWDispatchCardPro[Pro_Index_MAX];	//新帐号福利功能: 必赢局的发牌型概率

	uint32							m_lucky_win_uid;						//获取幸运值当前局的控制玩家

protected:
    CMaJiangLogic   m_gameLogic;                    // 游戏逻辑
    CCooling        m_coolRobot;                    // 机器人CD
    CMaJiangCfg     m_mjCfg;                        // 麻将配置
    WORD            m_wPlayerCount;                 // 玩家人数
    bool            m_bChangeHandleCards;           // 是否换牌过
	uint16			m_playType;						// 玩法

	int             m_stockTingCardZiMoPro; // 0 不触发库存输赢，> 0 玩家听牌自摸概率，< 0 机器人听牌自摸概率
};

#endif //SERVER_GAME_IMPLE_TABLE_H

