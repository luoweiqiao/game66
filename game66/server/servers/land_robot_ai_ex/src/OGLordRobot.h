#ifndef OGLORD_ROBOT_H_
#define OGLORD_ROBOT_H_

#include <vector>

class OGLordRobotAI;
class COGLordRbtAIClv;

enum ROBOT_LEVEL
{
    BASE_LEVEL = 0,
    SIMPLE_LEVEL = 1,
    MIDDLE_LEVEL = 2,
    HARD_LEVEL = 3,
    BEST_LEVEL = 4,
    HARD_LEVEL_2 = 5,
    BEST_LEVEL_2 = 6,
    TRUST_LEVEL = 7
};

class OGLordRobot
{
public:
    OGLordRobot();
    ~OGLordRobot();
    //【初始化】
    // 输入机器人智商级别
    bool RbtInSetLevel(int argLevel);

    //【发牌阶段】
    // 收到发牌消息
    bool RbtInInitCard(int argSeat, std::vector<int> argHandCard);    

    // 输入各个座位玩家的牌信息
    bool RbtInNtfCardInfo(std::vector<std::vector<int> > argHandCard);

    //【叫分阶段】
    // 收到玩家叫分信息
    bool RbtInCallScore(int argSeat, int argCallScore);

    // 请求给出叫分策略
    bool RbtOutGetCallScore(int &callScore);

    // 输入玩家抢地主信息
    bool RbtInSetGrabLord(int argSeat);

    // 输出抢地主策略
    bool RbtOutGetGrabLord(bool &grabLord);

    // 收到确定地主信息
    bool RbtInSetLord(int argLordSeat, std::vector<int> argReceiceCard);

    // 【出牌阶段】
    // 收到玩家牌信息
    bool RbtInTakeOutCard(int argSeat, std::vector<int> argCards);

    // 请求给出出牌策略
    bool RbtOutGetTakeOutCard(std::vector<int> &vecCards);

    // 【重置机器人】	
    // 重置机器人信息
    bool RbtResetData();

    // 用于断线续完
    // 设置地主和玩家座位
    bool RbtInSetSeat(int argMySeat, int argLordSeat);

    // 设置手牌和底牌
    bool RbtInSetCard(std::vector<int> argInitCard, std::vector<int> argReceiveCard);
    // 设置出牌记录
    bool RbtInTakeOutRecord(std::vector<int> argTakeOutSeat, std::vector<std::vector<int>> argTakeOutRecord);

    //【可选策略】
    // 输入玩家明牌信息 
    bool RbtInShowCard(int argShowSeat, std::vector<int> argHandCard);

    // 输出明牌策略 
    bool RbtOutShowCard(bool &showCard);

    // 输出加倍策略 
    bool RbtOutDoubleGame(bool &Double_game);
private:
    OGLordRobotAI *m_pRobot;
    COGLordRbtAIClv *m_pRobotClv;

public:
    OGLordRobot(const OGLordRobot&);
    OGLordRobot& operator=(const OGLordRobot&);
#if __cplusplus >= 201103L || (defined(_MSC_VER) && _MSC_VER >= 1800)
    OGLordRobot(OGLordRobot&&);
    OGLordRobot& operator=(OGLordRobot&&);
#endif
};

#endif
