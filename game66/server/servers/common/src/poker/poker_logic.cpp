//
// Created by toney on 16/4/19.
//

#include "poker_logic.h"

CPokerLogic::CPokerLogic()
{
}
CPokerLogic::~CPokerLogic()
{
}
//有效判断
bool CPokerLogic::IsValidCard(BYTE cbCardData)
{
    //获取属性
    BYTE cbCardColor = GetCardColor(cbCardData);
    BYTE cbCardValue = GetCardValue(cbCardData);
    //有效判断
    if ((cbCardData == 0x4E) || (cbCardData == 0x4F)) return true;
    if ((cbCardColor <= 0x30) && (cbCardValue >= 0x01) && (cbCardValue <= 0x0D)) return true;

    return false;
}
//构造扑克
BYTE CPokerLogic::MakeCardData(BYTE cbValueIndex, BYTE cbColorIndex)
{
    return (cbColorIndex << 4) | (cbValueIndex + 1);
}
// 排列组合
void CPokerLogic::Combination(const vector<BYTE>& t, int c,vector< vector<BYTE> >& results)
{
    //initial first combination like:1,1,0,0,0
    vector<int> vecInt(t.size(), 0);
    for(int i = 0; i < c; ++i){
        vecInt[i] = 1;
    }
    PrintCombRes(t,vecInt,results);
    for(int i = 0; i < (int)t.size() - 1; ++i)
    {
        if(vecInt[i] == 1 && vecInt[i+1] == 0)
        {
            //1. first exchange 1 and 0 to 0 1
            swap(vecInt[i], vecInt[i+1]);

            //2.move all 1 before vecInt[i] to left
            sort(vecInt.begin(),vecInt.begin() + i, compare);

            //after step 1 and 2, a new combination is exist
            PrintCombRes(t,vecInt,results);

            //try do step 1 and 2 from front
            i = -1;
        }
    }
}
void CPokerLogic::PrintCombRes(const vector<BYTE>& t,vector<int>& vecInt,vector< vector<BYTE> >& results)
{
    vector<BYTE> tmp;
    for(uint32 k = 0; k < vecInt.size(); ++k){
        if(vecInt[k] == 1){
            tmp.push_back(t[k]);
        }
    }
    results.push_back(tmp);
}
void CPokerLogic::SubVector(const vector<BYTE>& t,const vector<BYTE>& ex,vector<BYTE>& res)
{
    for(uint32 i=0;i<t.size();++i){
        bool isExist = false;
        for(uint32 j=0;j<ex.size();++j){
            if(t[i] == ex[j]){
                isExist = true;
                break;
            }
        }
        if(!isExist){
            res.push_back(t[i]);
        }
    }
}
bool CPokerLogic::compare(BYTE a, BYTE b)
{
    return (a > b) ? true : false;
}





















