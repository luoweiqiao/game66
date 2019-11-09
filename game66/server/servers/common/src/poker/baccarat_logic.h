//
// Created by toney on 16/8/27.
//

#ifndef SERVER_BACCARAT_LOGIC_H
#define SERVER_BACCARAT_LOGIC_H

#include "svrlib.h"
#include "poker_logic.h"

namespace game_baccarat
{
    //游戏逻辑
    class CBaccaratLogic : public CPokerLogic
    {
        //变量定义
    private:
        static const BYTE m_cbCardListData[52 * 8];                //扑克定义

        //函数定义
    public:
        //构造函数
        CBaccaratLogic();

        //析构函数
        virtual ~CBaccaratLogic();
    public:
        //混乱扑克
        void RandCardList(BYTE cbCardBuffer[], int32 cbBufferCount);

        //逻辑函数
    public:
        //获取牌点
        BYTE GetCardPip(BYTE cbCardData);

        //获取牌点
        BYTE GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount);
    };

};

#endif //SERVER_BACCARAT_LOGIC_H
