#ifndef OGLordRobotAI_h__
#define OGLordRobotAI_h__

// yz 2017-7-18
// 删除接口类继承关系
// 删除virtual

#pragma once
#include "Robot.h"

#include <vector>
#include <map>
#include <set>
//#include <hash_map>
//#include <algorithm>
//#include <numeric>

//#include <iostream>
#include "ddz_interface.h"

/*
enum ROBOT_LEVEL
{
    BASE_LEVEL = 0,
    SIMPLE_LEVEL = 1,
    MIDDLE_LEVEL = 2,
    HARD_LEVEL = 3,
    BEST_LEVEL = 4
};
*/

class OGLordRobotAI
{
public:
	//used for new robot
    LordRobot* robot;
	//LordRobot m_robot;
	int lord_score;

	OGLordRobotAI(void);
	~OGLordRobotAI(void);

    // 收到发牌消息
    bool RbtInInitCard(int argSeat,                        // 自己的座位号
        std::vector<int> argHandCard        // 发送的手牌
        );

    // 输入机器人智商级别
    bool RbtInSetLevel(int argLevel);                  // 机器人智商级别

    // 输入各个座位玩家的牌信息
    bool RbtInNtfCardInfo(std::vector<std::vector<int>> argHandCard
        // 各个座位玩家的手牌 0,1,2,3个元素分别表示第0,1,2号座位以及底牌的内容。
        );

    // 收到玩家叫分信息
    bool RbtInCallScore(int argSeat,                       // 座位号
        int argCallScore                   // 叫的分数
        );

    // 请求给出叫分策略
    bool RbtOutGetCallScore(int &callScore                 // 返回值引用 
        );

    // 输入玩家抢地主信息
    bool RbtInSetGrabLord(int argSeat                      // 座位号
        );

    // 输出抢地主策略
    bool RbtOutGetGrabLord(bool &grabLord                  // TRUE：抢地主 FALSE：不抢
        );

    // 收到确定地主信息
    bool RbtInSetLord(int argLordSeat,       // 地主座位
        std::vector<int> argReceiceCard      // 地主收到的底牌
        );


    // 【出牌阶段】
    // 收到玩家牌信息
    bool RbtInTakeOutCard(int argSeat,   // 座位号
        std::vector<int> argCards        // 牌内容
        );

    // 请求给出出牌策略
    bool RbtOutGetTakeOutCard(std::vector<int> &vecCards    // 返回的牌
        );


    // 【重置机器人】
    // 用于第二盘时复用机器人AI 以及断线续完
    // 重置机器人信息
    bool RbtResetData();

    // 设置地主和玩家座位
    bool RbtInSetSeat(int argMySeat,         // 自己的座位
        int argLordSeat                      // 地主的座位
        );

    // 设置手牌和底牌
    bool RbtInSetCard(std::vector<int> argInitCard,        // 自己初始化的牌
        std::vector<int> argReceiveCard      // 地主收到的底牌
        );
    // 设置出牌记录
    bool RbtInTakeOutRecord(std::vector<int> argTakeOutSeat,               // 历史出牌-座位记录
        std::vector<std::vector<int>> argTakeOutRecord // 历史出牌-牌记录
        );

	// 【其他】
	// 返回错误信息
	bool RbtOutGetLastError(int &errorCode);           // 返回当前错误码

    // 输入玩家明牌信息 
    bool RbtInShowCard(int argShowSeat, std::vector<int> argHandCard);

    // 输出明牌策略 
    bool RbtOutShowCard(bool &showCard);

    // 输出加倍策略 
    bool RbtOutDoubleGame(bool &Double_game);
private:

	void updateAiPosition();
	
	void sortHandMap();

	void sortHandMapLvl();

	void findLowestHand(Hand &hand);

	void findHigherHand(Hand &hand);

	void findHigherHandNotBomb(Hand &hand);

	void findMostCardsHandNotBomb(Hand &hand);

	void takeOutHand(Hand &hand, std::vector<int> &takeOutCards);

	void takeOutLvl0(Hand &hand);

	void takeOutLvl1(Hand &hand);

	void takeOutHighLvl(Hand &hand);

	bool isFree();

	bool isDanger();

	bool isGood(HandsMapSummary &hmSummary, int controlCount, int minOppNum);

	bool isFirstHalf();

	bool isGoodFarmer();

	void findChargeHandFirst(Hand &hand, bool typeCountFirst);

	void lordTakeOutFree(Hand &hand);

	void lordTakeOutFreeFarmerOnly1Card(Hand &hand);

	void lordTakeOutFreeFarmerOnly2Cards(Hand &hand);

	void lordTakeOutFreeNormal(Hand &hand);

	void lordTakeOutHigher(Hand &hand);

	void lordTakeOutHigherFarmerOnly1Card(Hand &hand);

	void lordTakeOutHigherFarmerOnly2Cards(Hand &hand);

	void lordTakeOutHigherNormal(Hand &hand);

	void lordTakeOutHigherRebuild(Hand &hand);

	void farmerTakeOutLordOnly1Card(Hand &hand);

	void farmerTakeOutLordOnly2Cards(Hand &hand);

	void farmerMustTakeOutLordOnly1Card(Hand &hand);

	void farmerMustTakeOutLordCharge(Hand &hand);

	void goodFarmerOverOtherFarmer(Hand &hand);

	void downGoodFarmerTakeOut(Hand &hand);

	void downBadFarmerTakeOut(Hand &hand);

	void upGoodFarmerTakeOut(Hand &hand);

	void upBadFarmerTakeOut(Hand &hand);

	void downFarmerTakeOutUpFarmerOnly1Card(Hand &hand);

	void findBestHigherHandFromMap(Hand &hand);

	void findBestHigherHandFromPoints(Hand &hand, bool force, bool lordOnly1Card);

	void farmerTakeOutWhenLordTakeOutHighSolo(Hand &hand);

	void refindForTrio(Hand &hand);

	bool isDangerLvl();

	bool isGoodLvl(HandsMapSummary &hmSummary, int minOppNum);

	void findKickerLvl(Hand &hand);

	bool containsHand(std::map<HandType, std::vector<Hand>> &allHands, Hand &hand);

	void lordTakeOutFreeLvl(Hand &hand);

	void lordTakeOutFreeFarmerOnly1CardLvl(Hand &hand);

	void lordTakeOutFreeFarmerOnly2CardsLvl(Hand &hand);

	void lordTakeOutFreeNormalLvl(Hand &hand);

	void lordTakeOutHigherLvl(Hand &hand);

	void lordTakeOutHigherFarmerOnly1CardLvl(Hand &hand);

	void lordTakeOutHigherFarmerOnly2CardsLvl(Hand &hand);

	void lordTakeOutHigherNormalLvl(Hand &hand);

	void lordTakeOutHigherRebuildLvl(Hand &hand);

	void farmerTakeOutLordOnly1CardLvl(Hand &hand);

	void farmerTakeOutLordOnly2CardsLvl(Hand &hand);

	void farmerMustTakeOutLordOnly1CardLvl(Hand &hand);

	void farmerMustTakeOutLordChargeLvl(Hand &hand);

	void goodFarmerOverOtherFarmerLvl(Hand &hand);

	void downGoodFarmerTakeOutLvl(Hand &hand);

	void downBadFarmerTakeOutLvl(Hand &hand);

	void upGoodFarmerTakeOutLvl(Hand &hand);

	void upBadFarmerTakeOutLvl(Hand &hand);

	void downFarmerTakeOutUpFarmerOnly1CardLvl(Hand &hand);

	void countHighHand(std::map<HandType, std::vector<Hand>> &allHands,
						std::map<HandType, int> &uniHighHands,
						std::map<HandType, int> &HighHands,
						std::map<HandType, int> &goodLowHands,
						int &trioCount);

	HandsMapSummary getHandsMapSummaryLvl(std::map<HandType, std::vector<Hand>> &allHands,
						std::map<HandType, int> &uniHighHands,
						std::map<HandType, int> &HighHands,
						std::map<HandType, int> &goodLowHands,
						int trioCount);

	int findOppHighestPoint(HandType type);

	int findOppHighestChain(HandType type, int len);

	int addAllValue(std::map<HandType, int> &handsCount);

	void findChargeHandFirstLvl(Hand &hand, bool typeCountFirst);

	bool mustHighLevel;
	
	int level;

	int aiSeat;

	int lordSeat;

	int downSeat;

	int upSeat;

	Position aiPosition;

	std::vector<int> leftoverCards;

	std::vector<int> aiCardsVec;

	CardsInfo playerInfo[PLAYER_NUM];

	int remainPoints[CARD_POINT_NUM];

	int otherPoints[CARD_POINT_NUM];

	std::map<HandType, std::vector<Hand>> handsMap;

	HandsMapSummary summary;

	std::map<HandType, int> uniHighHandCount;

	std::map<HandType, int> highHandCount;

	std::map<HandType, int> goodLowHandCount;

	int handsTrioCount;

	CardPoint lowestControl;

	int controlNum;

	int maxOppNumber;

	int minOppNumber;

    int maxScore;

	int curScore;

	int curCaller;
	
	Hand curHand;

	int curHandSeat;

	std::vector<std::pair<int, int>> callHistory;

	std::vector<std::pair<int, Hand>> history;

	int errCode;
};

#endif // OGLordRobotAI_h__
