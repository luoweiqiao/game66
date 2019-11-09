#include "OGLordRobot.h"
#include "OGLordRobotAI.h"
#include "OGLordRbtAIClv.h"

using namespace std;

OGLordRobot::OGLordRobot()
{
    m_pRobot = new OGLordRobotAI();
    m_pRobotClv = new COGLordRbtAIClv();
}

OGLordRobot::~OGLordRobot()
{
    delete m_pRobot;
    m_pRobot = NULL;
    delete m_pRobotClv;
    m_pRobotClv = NULL;
}

bool OGLordRobot::RbtInSetLevel(int argLevel)
{
    if (argLevel == TRUST_LEVEL)
    {
        argLevel = BEST_LEVEL_2;
        //delete m_pRobot;
        //m_pRobot = NULL;
        //delete m_pRobotClv;
        //m_pRobotClv = new COGLordRbtAIClv();

        //return m_pRobotClv ? m_pRobotClv->RbtInSetLevel(0) : false;
    }

    if (BASE_LEVEL <= argLevel && argLevel <= BEST_LEVEL)
    {
        delete m_pRobotClv;
        m_pRobotClv = NULL;
        if (!m_pRobot)
        {
            m_pRobot = new OGLordRobotAI();
        }
        return m_pRobot ? m_pRobot->RbtInSetLevel(argLevel) : false;
    }
    else
    {
        delete m_pRobot;
        m_pRobot = NULL;
        if (!m_pRobotClv)
        {
            m_pRobotClv = new COGLordRbtAIClv();
        }
        return m_pRobotClv ? m_pRobotClv->RbtInSetLevel(argLevel - BEST_LEVEL + 2) : false; //TODO
    }
}

bool OGLordRobot::RbtInInitCard(int argSeat, /* 自己的座位号 */ std::vector<int> argHandCard /* 发送的手牌 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInInitCard(argSeat, argHandCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInInitCard(argSeat, argHandCard) : false;
}

bool OGLordRobot::RbtInNtfCardInfo(std::vector<std::vector<int>> argHandCard /* 各个座位玩家的手牌 0,1,2,3个元素分别表示第0,1,2号座位以及底牌的内容。 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInNtfCardInfo(argHandCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInNtfCardInfo(argHandCard) : false;
}

bool OGLordRobot::RbtInCallScore(int argSeat, /* 座位号 */ int argCallScore /* 叫的分数 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInCallScore(argSeat, argCallScore);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInCallScore(argSeat, argCallScore) : false;
}

bool OGLordRobot::RbtOutGetCallScore(int &callScore /* 返回值引用 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutGetCallScore(callScore);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutGetCallScore(callScore, delay) : false;
}

bool OGLordRobot::RbtInSetGrabLord(int argSeat /* 座位号 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetGrabLord(argSeat);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetGrabLord(argSeat) : false;
}

bool OGLordRobot::RbtOutGetGrabLord(bool &grabLord /* TRUE：抢地主 FALSE：不抢 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutGetGrabLord(grabLord);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutGetGrabLord(grabLord, delay) : false;
}

bool OGLordRobot::RbtInSetLord(int argLordSeat, /* 地主座位 */ std::vector<int> argReceiceCard /* 地主收到的底牌 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetLord(argLordSeat, argReceiceCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetLord(argLordSeat, argReceiceCard) : false;
}

bool OGLordRobot::RbtInTakeOutCard(int argSeat, /* 座位号 */ std::vector<int> argCards /* 牌内容 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInTakeOutCard(argSeat, argCards);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInTakeOutCard(argSeat, argCards) : false;
}

bool OGLordRobot::RbtOutGetTakeOutCard(std::vector<int> &vecCards /* 返回的牌 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutGetTakeOutCard(vecCards);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutGetTakeOutCard(vecCards, delay) : false;
}

bool OGLordRobot::RbtResetData()
{
    if (m_pRobot)
    {
        return m_pRobot->RbtResetData();
    }
    return m_pRobotClv ? m_pRobotClv->RbtResetData() : false;
}

bool OGLordRobot::RbtInSetSeat(int argMySeat, /* 自己的座位 */ int argLordSeat /* 地主的座位 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetSeat(argMySeat, argLordSeat);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetSeat(argMySeat, argLordSeat) : false;
}

bool OGLordRobot::RbtInSetCard(std::vector<int> argInitCard, /* 自己初始化的牌 */ std::vector<int> argReceiveCard /* 地主收到的底牌 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInSetCard(argInitCard, argReceiveCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInSetCard(argInitCard, argReceiveCard) : false;
}

bool OGLordRobot::RbtInTakeOutRecord(std::vector<int> argTakeOutSeat, /* 历史出牌-座位记录 */ std::vector<std::vector<int>> argTakeOutRecord /* 历史出牌-牌记录 */)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInTakeOutRecord(argTakeOutSeat, argTakeOutRecord);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInTakeOutRecord(argTakeOutSeat, argTakeOutRecord) : false;
}

bool OGLordRobot::RbtInShowCard(int argShowSeat, std::vector<int> argHandCard)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtInShowCard(argShowSeat, argHandCard);
    }
    return m_pRobotClv ? m_pRobotClv->RbtInShowCard(argShowSeat, argHandCard) : false;
}

bool OGLordRobot::RbtOutShowCard(bool &showCard)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutShowCard(showCard);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutShowCard(showCard, delay) : false;
}

bool OGLordRobot::RbtOutDoubleGame(bool &Double_game)
{
    if (m_pRobot)
    {
        return m_pRobot->RbtOutDoubleGame(Double_game);
    }
    int delay = 0;
    return m_pRobotClv ? m_pRobotClv->RbtOutDoubleGame(Double_game, delay) : false;
}
