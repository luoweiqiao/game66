//
// Created by toney on 16/4/19.
//

#ifndef SERVER_POKER_LOGIC_H
#define SERVER_POKER_LOGIC_H

#include "svrlib.h"

// 扑克数值掩码
#define	MASK_COLOR	    0xF0	//花色掩码
#define	MASK_VALUE	    0x0F	//数值掩码

#define FULL_POKER_COUNT  54    // 一副扑克牌数目
#define INVALID_CHAIR     0xFF  //


class CPokerLogic
{
public:
    CPokerLogic();
    ~CPokerLogic();

    //获取扑克数值
    virtual uint8    GetCardValue(uint8 cbCardData) { return cbCardData&MASK_VALUE; }
    //获取扑克花色
    virtual uint8    GetCardColor(uint8 cbCardData) { return cbCardData&MASK_COLOR; }
    //获取扑克花色值
    virtual uint8    GetCardColorValue(uint8 cbCardData) { return (cbCardData&MASK_COLOR)>>4; }

    //有效判断
    virtual bool     IsValidCard(BYTE cbCardData);
    //构造扑克
    virtual BYTE     MakeCardData(BYTE cbValueIndex, BYTE cbColorIndex);

    // 排列组合(注意控制组合结果值数量)
    void    Combination(const vector<BYTE>& t,int c,vector< vector<BYTE> >& results);
    void    PrintCombRes(const vector<BYTE>& t,vector<int>& vecInt,vector< vector<BYTE> >& results);
    void    SubVector(const vector<BYTE>& t,const vector<BYTE>& ex,vector<BYTE>& res);

    static  bool compare(BYTE a, BYTE b);


};
















#endif //SERVER_POKER_LOGIC_H
