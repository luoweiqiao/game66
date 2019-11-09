//
// Created by toney on 16/8/27.
//

#ifndef SERVER_PAIJIU_LOGIC_H
#define SERVER_PAIJIU_LOGIC_H

#include "svrlib.h"
#include "poker/poker_logic.h"

namespace game_paijiu
{

    //扑克类型
    enum emPAIJIU_CARD_TYPE
    {
        CT_ERROR					= 0,									//错误类型
        CT_POINT_0					= 29,									//憋十
        CT_POINT_1					= 28,									//一点
        CT_POINT_2					= 27,									//两点
        CT_POINT_3					= 26,									//三点
        CT_POINT_4					= 25,									//四点
        CT_POINT_5					= 24,									//五点
        CT_POINT_6					= 23,									//六点
        CT_POINT_7					= 22,									//七点
        CT_POINT_8					= 21,									//八点
        CT_POINT_9					= 20,									//九点
        CT_SPECIAL_19				= 19,									//地杠
        CT_SPECIAL_18				= 18,									//天杠
        CT_SPECIAL_17				= 17,									//天九王
        CT_SPECIAL_16				= 16,									//杂五
        CT_SPECIAL_15				= 15,									//杂七
        CT_SPECIAL_14				= 14,									//杂八
        CT_SPECIAL_13				= 13,									//杂九
        CT_SPECIAL_12				= 12,									//双幺五
        CT_SPECIAL_11				= 11,									//双铜锤
        CT_SPECIAL_10				= 10,									//双红头
        CT_SPECIAL_9				= 9,								    //双斧头
        CT_SPECIAL_8				= 8,									//双板凳
        CT_SPECIAL_7				= 7,									//双长
        CT_SPECIAL_6				= 6,									//双梅
        CT_SPECIAL_5				= 5,									//双和
        CT_SPECIAL_4				= 4,									//双人
        CT_SPECIAL_3				= 3,									//双地
        CT_SPECIAL_2				= 2,									//双天
        CT_SPECIAL_1				= 1,									//至尊宝
    };

    //排序类型
    const static int32 	ST_VALUE = 1;									//数值排序
    const static int32	ST_LOGIC = 2;									//逻辑排序
    //扑克数目
    const static  int32 CARD_COUNT = 32;								//扑克数目
    //数值掩码
    const static BYTE LOGIC_MASK_COLOR = 0xF0;							//花色掩码
    const static BYTE LOGIC_MASK_VALUE = 0x0F;							//数值掩码

    //游戏逻辑
    class CPaijiuLogic : public CPokerLogic
    {
        //变量定义
    public:
        static const BYTE	m_cbCardListData[CARD_COUNT];		//扑克定义

        //函数定义
    public:
        //构造函数
        CPaijiuLogic();

        //析构函数
        virtual ~CPaijiuLogic();
    public:
        //获取数值
        BYTE GetCardValue(BYTE cbCardData) { return cbCardData&LOGIC_MASK_VALUE; }
        //获取花色
        BYTE GetCardColor(BYTE cbCardData) { return (cbCardData&LOGIC_MASK_COLOR)>>4; }

        //混乱扑克
        void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
        //排列扑克 从大到小排序
        void SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType);

        //逻辑函数
    public:
        //获取牌点
        BYTE GetCardListPip(const BYTE cbCardData[], BYTE cbCardCount);
        //获取牌型
        BYTE GetCardType(const BYTE cbCardData[], BYTE cbCardCount);
        //大小比较
        int CompareCard(const BYTE cbFirstCardData[], BYTE cbFirstCardCount,const BYTE cbNextCardData[], BYTE cbNextCardCount);
        //逻辑大小，返回的数值越大，逻辑值就越大
        BYTE GetCardLogicValue(BYTE cbCardData);





    };

};

#endif //SERVER_BACCARAT_LOGIC_H
