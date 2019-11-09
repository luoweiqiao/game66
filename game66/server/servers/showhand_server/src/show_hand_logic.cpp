//
// Created by toney on 16/4/12.
//
#include "show_hand_logic.h"
#include "svrlib.h"
#include "math/rand_table.h"

using namespace svrlib;

namespace game_show_hand
{

//#define    MAGIC_A_CARD                                //A是否可变换成7

//扑克数据
    BYTE CShowHandLogic::m_cbCardListData[MAX_SHOWHAND_POKER] =
            {
                    0x01, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,                                    //方块 A - K
                    0x11, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,                                    //梅花 A - K
                    0x21, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,                                    //红桃 A - K
                    0x31, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D,                                    //黑桃 A - K
            };
//构造函数
    CShowHandLogic::CShowHandLogic() {
    }

//析构函数
    CShowHandLogic::~CShowHandLogic() {
    }

//获取类型
    BYTE CShowHandLogic::GetCardGenre(BYTE cbCardData[], BYTE cbCardCount) {
		SortCardList(cbCardData, cbCardCount);
        //简单牌形
        switch (cbCardCount)
        {
            case 1: //单牌
            {
                return CT_SINGLE;
            }
            case 2: //对牌
            {
                return (GetCardLogicValue(cbCardData[0]) == GetCardLogicValue(cbCardData[1])) ? CT_ONE_DOUBLE
                                                                                              : CT_SINGLE;
            }
            case 3: //三张
            {
                if(GetCardLogicValue(cbCardData[0]) == GetCardLogicValue(cbCardData[2])) 
                    return CT_THREE_TIAO;
                if(GetCardLogicValue(cbCardData[0]) == GetCardLogicValue(cbCardData[1]) ||
                   GetCardLogicValue(cbCardData[1]) == GetCardLogicValue(cbCardData[2])){
                    return CT_ONE_DOUBLE;
                }
                return CT_SINGLE;                   
            } 
            
        }
        //五条类型
        if (cbCardCount >= 4) {
            //变量定义
            bool cbSameColor = true, bLineCard = true;
            BYTE cbFirstColor = GetCardColor(cbCardData[0]);
            BYTE cbFirstValue = GetCardLogicValue(cbCardData[0]);

            BYTE i = 1;
            //牌形分析
            for (i = 1; i < cbCardCount; i++) {
                //数据分析
                if (GetCardColor(cbCardData[i]) != cbFirstColor) cbSameColor = false;
                if (cbFirstValue != (GetCardLogicValue(cbCardData[i]) + i)) bLineCard = false;

                //结束判断
                if ((cbSameColor == false) && (bLineCard == false)) break;
            }

//#ifdef    MAGIC_A_CARD
//            //特殊
//            if (!bLineCard && GetCardValue(cbCardData[0]) == 1) {
//                //A J 10 9 8特殊
//                BYTE cbMinusCount = cbCardCount == 4 ? 3 : 2;
//                for (i = 1; i < cbCardCount; i++) {
//                    if ((cbFirstValue - cbMinusCount) != (GetCardLogicValue(cbCardData[i]) + i)) break;
//                }
//                if (i == cbCardCount) bLineCard = true;
//            }
//#endif

            //顺子类型
            if ((cbSameColor == false) && (bLineCard == true)) return CT_SHUN_ZI;

            //同花类型
            if ((cbSameColor == true) && (bLineCard == false)) return CT_TONG_HUA;

            //同花顺类型
            if ((cbSameColor == true) && (bLineCard == true)) return CT_TONG_HUA_SHUN;
        }

        //扑克分析
        tagAnalyseResult AnalyseResult;
        AnalysebCardData(cbCardData, cbCardCount, AnalyseResult);

        //四条类型
        if (AnalyseResult.cbFourCount == 1) return CT_TIE_ZHI;

        //两对类型
        if (AnalyseResult.cbDoubleCount == 2) return CT_TWO_DOUBLE;

        //对牌类型
        if ((AnalyseResult.cbDoubleCount == 1) && (AnalyseResult.cbThreeCount == 0)) return CT_ONE_DOUBLE;

        //葫芦类型
        if (AnalyseResult.cbThreeCount == 1) return (AnalyseResult.cbDoubleCount == 1) ? CT_HU_LU : CT_THREE_TIAO;

        return CT_SINGLE;
    }

//排列扑克
    void CShowHandLogic::SortCardList(BYTE cbCardData[], BYTE cbCardCount) {
        //转换数值
        if(cbCardCount < 2)
            return;
        BYTE cbLogicValue[MAX_COUNT];
        for (BYTE i = 0; i < cbCardCount; i++) {
            cbLogicValue[i] = GetCardLogicValue(cbCardData[i]);
        }

        //排序操作
        bool bSorted = true;
        BYTE cbTempData, bLast = cbCardCount - 1;
        do {
            bSorted = true;
            for (BYTE i = 0; i < bLast; i++) {
                if ((cbLogicValue[i] < cbLogicValue[i + 1]) ||
                    ((cbLogicValue[i] == cbLogicValue[i + 1]) && (cbCardData[i] < cbCardData[i + 1]))) {
                    //交换位置
                    cbTempData = cbCardData[i];
                    cbCardData[i] = cbCardData[i + 1];
                    cbCardData[i + 1] = cbTempData;
                    cbTempData = cbLogicValue[i];
                    cbLogicValue[i] = cbLogicValue[i + 1];
                    cbLogicValue[i + 1] = cbTempData;
                    bSorted = false;
                }
            }
            bLast--;
        } while (bSorted == false);

        return;
    }
    //获取牌型值
    BYTE CShowHandLogic::GetCardGenreValue(BYTE cbCardData[], BYTE cbCardCount)
    {
        //获取类型
        BYTE cbGenre = GetCardGenre(cbCardData, cbCardCount);
        BYTE retValue = 0;
        //类型对比
        switch(cbGenre) {
            case CT_SINGLE:            //单牌
            {
                for(BYTE i = 0; i < cbCardCount; i++){
                    BYTE cbValue = GetCardLogicValue(cbCardData[i]);
                    if(cbValue > retValue){
                        retValue = cbValue;
                    }
                }
                return retValue;
            }
            case CT_HU_LU:              //葫芦
            case CT_TIE_ZHI:            //铁支
            case CT_ONE_DOUBLE:         //对子
            case CT_TWO_DOUBLE:         //两对
            case CT_THREE_TIAO:         //三条
            {
                //分析扑克
                tagAnalyseResult AnalyseResult;
                 AnalysebCardData(cbCardData, cbCardCount, AnalyseResult);
                //四条数值
                if (AnalyseResult.cbFourCount > 0) {
                    retValue = AnalyseResult.cbFourLogicVolue[0];
                    return retValue;
                }
                //三条数值
                if (AnalyseResult.cbThreeCount > 0) {
                    retValue = AnalyseResult.cbThreeLogicVolue[0];
                    return retValue;
                }
                //对子数值
                for (BYTE i = 0; i < AnalyseResult.cbDoubleCount; i++) {
                    BYTE cbValue = AnalyseResult.cbDoubleLogicVolue[i];
                    if (cbValue > retValue){
                        retValue = cbValue;
                    }
                    if( i == AnalyseResult.cbDoubleCount-1){
                        return retValue;
                    }
                }
                //散牌数值
                for (BYTE i = 0; i < AnalyseResult.cbSignedCount; i++) {
                    BYTE cbValue = AnalyseResult.cbSignedLogicVolue[i];
                    if ( cbValue > retValue){
                        retValue = cbValue;
                    }
                    if(i == AnalyseResult.cbSignedCount-1){
                        return retValue;
                    }
                }
                break;
            }
            case CT_SHUN_ZI:         //顺子
            case CT_TONG_HUA:        //同花
            case CT_TONG_HUA_SHUN:   //同花顺
            {
                //数值判断
                BYTE cbValue = GetCardLogicValue(cbCardData[0]);

//#ifdef    MAGIC_A_CARD
//                //特殊列:A J 10 9 8
//                if (cbGenre != CT_TONG_HUA) {
//                    BYTE cbMinusCount = cbCardCount == 5 ? 3 : 4;
//                    if (GetCardValue(cbCardData[0]) == 1 &&
//                        GetCardLogicValue(cbCardData[0]) - cbMinusCount == GetCardLogicValue(cbCardData[1]))
//                        cbValue = GetCardLogicValue(cbCardData[1]);
//                }
//#endif
                retValue = cbValue;
                return retValue;
            }
        }

        //错误断言
        ASSERT(false);

        return retValue;
    }
    //获得牌面胜率
    uint32 CShowHandLogic::GetWinPro(BYTE cbFirstCardData[],BYTE firstCount,BYTE cbNextCardData[],BYTE nextCount,bool type5,bool type3)
    {
        uint32 winCount   = 0;
        uint32 totalCount = 0;
        uint8  selectFlag[MAX_SHOWHAND_POKER];
        memset(selectFlag,0,sizeof(selectFlag));

        // 设置手牌标记
        for(uint8 i=0;i<firstCount;++i){
            for(uint8 j=0;j<MAX_SHOWHAND_POKER;++j){
                if(cbFirstCardData[i] == m_cbCardListData[j]){
                    selectFlag[j] = 1;
                    continue;
                }
            }
        }
        for(uint8 i=0;i<nextCount;++i){
            for(uint8 j=0;j<MAX_SHOWHAND_POKER;++j){
                if(cbNextCardData[i] == m_cbCardListData[j]){
                    selectFlag[j] = 2;
                    continue;
                }
            }
        }
        // 在剩下的牌里面遍历选择牌型组合
        vector<BYTE> pokerPool;
        for(uint8 i=0;i<MAX_SHOWHAND_POKER;++i){
            if(selectFlag[i] == 0){
                pokerPool.push_back(m_cbCardListData[i]);
            }
        }
        // 获取所有组合
        vector< vector<BYTE> > resOne;
        int combCount1 = (MAX_COUNT-firstCount);
        int combCount2 = (MAX_COUNT-nextCount);
        
        if(combCount1 > 0 && combCount2 > 0)// 两边选牌
        {
            Combination(pokerPool,combCount1,resOne);//
            // 遍历玩家1的组合，获得玩家2的组合
            for(uint32 i=0;i<resOne.size();++i)
            {
                vector<BYTE> subRes;
                SubVector(pokerPool,resOne[i],subRes);// 玩家1组合剩余牌内抽
                vector< vector<BYTE> > resTwo;
                Combination(subRes,combCount2,resTwo);
                for(uint32 j=0;j<resTwo.size();++j)
                {
                    BYTE firstCard[MAX_COUNT];
                    BYTE nextCard[MAX_COUNT];
                    memcpy(firstCard,cbFirstCardData,firstCount);
                    memcpy(nextCard,cbNextCardData,nextCount);
                    for(uint8 n=0;n<resOne[i].size();++n)
                    {
                        firstCard[firstCount+n] = resOne[i][n];
                    }
                    for(uint8 n=0;n<resTwo[j].size();++n)
                    {
                        nextCard[nextCount+n] = resTwo[j][n];
                    }
                    if(IsMatchedCardType(firstCard,MAX_COUNT,type5,type3) == false
                    || IsMatchedCardType(nextCard,MAX_COUNT,type5,type3) == false)
                    {
                        continue;                        
                    }
                    SortCardList(firstCard,MAX_COUNT);
                    SortCardList(nextCard,MAX_COUNT);

                    if(CompareCard(firstCard,nextCard,MAX_COUNT)){
                        winCount++;
                    }
                    totalCount++;
                }
            }
        }
        else if(combCount1 == 0 && combCount2 > 0)
        {
            vector< vector<BYTE> > resTwo;
            Combination(pokerPool,combCount2,resTwo);
            for(uint32 j=0;j<resTwo.size();++j)
            {
                BYTE firstCard[MAX_COUNT];
                BYTE nextCard[MAX_COUNT];
                memcpy(firstCard,cbFirstCardData,firstCount);
                memcpy(nextCard,cbNextCardData,nextCount);                    
                for(uint8 n=0;n<resTwo[j].size();++n)
                {
                    nextCard[nextCount+n] = resTwo[j][n];
                }
                SortCardList(firstCard,MAX_COUNT);
                SortCardList(nextCard,MAX_COUNT);
                if(CompareCard(firstCard,nextCard,MAX_COUNT)){
                    winCount++;
                }
                totalCount++;
            }                           
        }          
        
        LOG_DEBUG("胜率计算结果:first:%d,next:%d,win:%d--total:%d,type5:%d,type3:%d",firstCount,nextCount,winCount,totalCount,type5,type3);
        if(totalCount > 1){
            LOG_DEBUG("胜率计算结果::%d",winCount * 100/totalCount);
            return winCount * 100/totalCount;
        }
        return 0;
    }
    //是否匹配做牌牌型
    bool CShowHandLogic::IsMatchedCardType(BYTE cbCardData[],BYTE count,bool type5,bool type3)
    {
        SortCardList(cbCardData,MAX_COUNT);
        uint8 cardType5 = GetCardGenre(cbCardData,MAX_COUNT);
        if(type5 && cardType5 < CT_ONE_DOUBLE)
            return false;
        
        uint8 tmp[3];
        memcpy(tmp,cbCardData,3);
        SortCardList(tmp,3);			    
        uint8 cardType3 = GetCardGenre(tmp,3);
		if (type3 && cardType3 < CT_ONE_DOUBLE)
		{
			return false;
		}
        
	    return true;      
    }

	//随机牌牌型
	bool CShowHandLogic::GetRandCardType(BYTE cbCardData[], BYTE count, bool & type5, bool & type3)
	{
		SortCardList(cbCardData, MAX_COUNT);
		uint8 cardType5 = GetCardGenre(cbCardData, MAX_COUNT);
		if (cardType5 < CT_ONE_DOUBLE) {
			type5 = false;
		}
		else {
			type5 = true;
		}
		uint8 tmp[3];
		memcpy(tmp, cbCardData, 3);
		SortCardList(tmp, 3);
		uint8 cardType3 = GetCardGenre(tmp, 3);
		if (cardType3 < CT_ONE_DOUBLE)
		{
			type3 = false;
		}
		else {
			type3 = true;
		}
		return true;
	}

    bool CShowHandLogic::CanPlayFirstCardValue(BYTE cbHandCardData[],BYTE cbCardCount)
    {
        bool  sameColor = false; // 同色
        bool  duizi     = false; // 对子
        bool  lianshun  = false; // 连顺          
        
        if(GetCardColor(cbHandCardData[0]) == GetCardColor(cbHandCardData[1])){
            sameColor = true;
        }
        int32 values[2];
        values[0] = GetCardLogicValue(cbHandCardData[0]);
        values[1] = GetCardLogicValue(cbHandCardData[1]);
        if(values[0] == values[1]){
            duizi = true;
        }
        if(abs(values[0]-values[1]) == 1){
            lianshun = true;
        }
        if(sameColor || duizi || lianshun)
            return true;
        
        return false;           
    }
    //洗牌换牌
    void CShowHandLogic::GetLeftCard(BYTE cbCardBuffer[],BYTE cbBufferCount,vector<uint8>& leftCards)
    {
        BYTE datalen = getArrayLen(m_cbCardListData);
        BYTE cbCardData[datalen];
        memcpy(cbCardData, m_cbCardListData, sizeof(m_cbCardListData));
        for(uint16 i=0;i<datalen;++i)
        {
            for(uint16 j=0;j<cbBufferCount;++j){
                if(cbCardData[i] == cbCardBuffer[j])
                {
                    cbCardData[i] = 0;
                }
            }              
        }
        for(uint16 i=0;i<datalen;++i)
        {
            if(cbCardData[i] != 0)
            {
                leftCards.push_back(cbCardData[i]);
            }                          
        }           
    }    
    //混乱扑克
    void CShowHandLogic::RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount) {

        //混乱准备
		BYTE datalen = MAX_SHOWHAND_POKER;// getArrayLen(m_cbCardListData);
        BYTE cbCardData[MAX_SHOWHAND_POKER];
        memcpy(cbCardData, m_cbCardListData, sizeof(m_cbCardListData));

        //混乱扑克
        BYTE cbRandCount = 0, cbPosition = 0;
        do
        {
            cbPosition=(BYTE)(g_RandGen.RandUInt()%(datalen-cbRandCount));
            cbCardBuffer[cbRandCount++]=cbCardData[cbPosition];
            cbCardData[cbPosition]=cbCardData[datalen-cbRandCount];
        } while (cbRandCount<cbBufferCount);

        return;
    }

//逻辑数值
    BYTE CShowHandLogic::GetCardLogicValue(BYTE cbCardData) {
        //转换数值
        BYTE cbCardValue = GetCardValue(cbCardData);
        BYTE cbLogicValue = (cbCardValue <= 2) ? (cbCardValue + 13) : cbCardValue;

        return cbLogicValue;
    }

//对比扑克
    bool CShowHandLogic::CompareCard(BYTE cbFirstCardData[], BYTE cbNextCardData[], BYTE cbCardCount) {
        //获取类型
        BYTE cbNextGenre = GetCardGenre(cbNextCardData, cbCardCount);
        BYTE cbFirstGenre = GetCardGenre(cbFirstCardData, cbCardCount);

        //类型判断
        if (cbFirstGenre != cbNextGenre) return (cbFirstGenre > cbNextGenre);

        //类型对比
        switch (cbFirstGenre) {
            case CT_SINGLE:            //单牌
            {
                //对比数值
                for (BYTE i = 0; i < cbCardCount; i++) {
                    BYTE cbNextValue = GetCardLogicValue(cbNextCardData[i]);
                    BYTE cbFirstValue = GetCardLogicValue(cbFirstCardData[i]);
                    if (cbFirstValue != cbNextValue) return cbFirstValue > cbNextValue;
                }

                //对比花色
                return GetCardColor(cbFirstCardData[0]) > GetCardColor(cbNextCardData[0]);
            }
            case CT_HU_LU:              //葫芦
            case CT_TIE_ZHI:            //铁支
            case CT_ONE_DOUBLE:         //对子
            case CT_TWO_DOUBLE:         //两对
            case CT_THREE_TIAO:         //三条
            {
                //分析扑克
                tagAnalyseResult AnalyseResultNext;
                tagAnalyseResult AnalyseResultFirst;
                AnalysebCardData(cbNextCardData, cbCardCount, AnalyseResultNext);
                AnalysebCardData(cbFirstCardData, cbCardCount, AnalyseResultFirst);

                //四条数值
                if (AnalyseResultFirst.cbFourCount > 0) {
                    BYTE cbNextValue = AnalyseResultNext.cbFourLogicVolue[0];
                    BYTE cbFirstValue = AnalyseResultFirst.cbFourLogicVolue[0];
                    return cbFirstValue > cbNextValue;
                }

                //三条数值
                if (AnalyseResultFirst.cbThreeCount > 0) {
                    BYTE cbNextValue = AnalyseResultNext.cbThreeLogicVolue[0];
                    BYTE cbFirstValue = AnalyseResultFirst.cbThreeLogicVolue[0];
                    return cbFirstValue > cbNextValue;
                }

                //对子数值
                for (BYTE i = 0; i < AnalyseResultFirst.cbDoubleCount; i++) {
                    BYTE cbNextValue = AnalyseResultNext.cbDoubleLogicVolue[i];
                    BYTE cbFirstValue = AnalyseResultFirst.cbDoubleLogicVolue[i];
                    if (cbFirstValue != cbNextValue) return cbFirstValue > cbNextValue;
                }

                //散牌数值
                for (BYTE i = 0; i < AnalyseResultFirst.cbSignedCount; i++) {
                    BYTE cbNextValue = AnalyseResultNext.cbSignedLogicVolue[i];
                    BYTE cbFirstValue = AnalyseResultFirst.cbSignedLogicVolue[i];
                    if (cbFirstValue != cbNextValue) return cbFirstValue > cbNextValue;
                }

                //对子花色
                if (AnalyseResultFirst.cbDoubleCount > 0) {
                    BYTE cbNextColor = GetCardColor(AnalyseResultNext.cbDoubleCardData[0]);
                    BYTE cbFirstColor = GetCardColor(AnalyseResultFirst.cbDoubleCardData[0]);
                    return cbFirstColor > cbNextColor;
                }

                //散牌花色
                if (AnalyseResultFirst.cbSignedCount > 0) {
                    BYTE cbNextColor = GetCardColor(AnalyseResultNext.cbSignedCardData[0]);
                    BYTE cbFirstColor = GetCardColor(AnalyseResultFirst.cbSignedCardData[0]);
                    return cbFirstColor > cbNextColor;
                }

                break;
            }
            case CT_SHUN_ZI:        //顺子
            case CT_TONG_HUA:        //同花
            case CT_TONG_HUA_SHUN:    //同花顺
            {

//#ifdef    MAGIC_A_CARD
//                //特殊列:A J 10 9 8
//                if (cbFirstGenre != CT_TONG_HUA) {
//                    BYTE cbMinusCount = cbCardCount == 5 ? 3 : 4;
//                    if (GetCardValue(cbNextCardData[0]) == 1 &&
//                        GetCardLogicValue(cbNextCardData[0]) - cbMinusCount == GetCardLogicValue(cbNextCardData[1]))
//                        cbNextValue = GetCardLogicValue(cbNextCardData[1]);
//                    if (GetCardValue(cbFirstCardData[0]) == 1 &&
//                        GetCardLogicValue(cbFirstCardData[0]) - cbMinusCount == GetCardLogicValue(cbFirstCardData[1]))
//                        cbFirstValue = GetCardLogicValue(cbFirstCardData[1]);
//                }
//#endif
//数值判断
				//BYTE cbNextValue = GetCardLogicValue(cbNextCardData[0]);
				//BYTE cbFirstValue = GetCardLogicValue(cbFirstCardData[0]);
				//if (cbFirstValue != cbNextValue) return (cbFirstValue > cbNextValue);

				for (BYTE i = 0; i < cbCardCount; i++)
				{
					BYTE cbNextValue = GetCardLogicValue(cbNextCardData[i]);
					BYTE cbFirstValue = GetCardLogicValue(cbFirstCardData[i]);
					if (cbFirstValue != cbNextValue) return cbFirstValue > cbNextValue;
				}
                //花色判断
                BYTE cbNextColor = GetCardColor(cbNextCardData[0]);
                BYTE cbFirstColor = GetCardColor(cbFirstCardData[0]);

                return (cbFirstColor > cbNextColor);
            }
        }

        //错误断言
        ASSERT(false);

        return false;
    }

//分析扑克
    void CShowHandLogic::AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult &AnalyseResult) {
        //设置结果
        memset(&AnalyseResult,0, sizeof(AnalyseResult));

        //扑克分析
        for (BYTE i = 0; i < cbCardCount; i++) {
            //变量定义
            BYTE cbSameCount = 1;
            BYTE cbSameCardData[4] = {cbCardData[i], 0, 0, 0};
            BYTE cbLogicValue = GetCardLogicValue(cbCardData[i]);

            //获取同牌
            for (int j = i + 1; j < cbCardCount; j++) {
                //逻辑对比
                if (GetCardLogicValue(cbCardData[j]) != cbLogicValue) break;

                //设置扑克
                cbSameCardData[cbSameCount++] = cbCardData[j];
            }

            //保存结果
            switch (cbSameCount) {
                case 1:        //单张
                {
                    AnalyseResult.cbSignedLogicVolue[AnalyseResult.cbSignedCount] = cbLogicValue;
                    memcpy(&AnalyseResult.cbSignedCardData[(AnalyseResult.cbSignedCount++) * cbSameCount],
                               cbSameCardData, cbSameCount);
                    break;
                }
                case 2:        //两张
                {
                    AnalyseResult.cbDoubleLogicVolue[AnalyseResult.cbDoubleCount] = cbLogicValue;
                    memcpy(&AnalyseResult.cbDoubleCardData[(AnalyseResult.cbDoubleCount++) * cbSameCount],
                               cbSameCardData, cbSameCount);
                    break;
                }
                case 3:        //三张
                {
                    AnalyseResult.cbThreeLogicVolue[AnalyseResult.cbThreeCount] = cbLogicValue;
                    memcpy(&AnalyseResult.cbThreeCardData[(AnalyseResult.cbThreeCount++) * cbSameCount],
                               cbSameCardData, cbSameCount);
                    break;
                }
                case 4:        //四张
                {
                    AnalyseResult.cbFourLogicVolue[AnalyseResult.cbFourCount] = cbLogicValue;
                    memcpy(&AnalyseResult.cbFourCardData[(AnalyseResult.cbFourCount++) * cbSameCount],
                               cbSameCardData, cbSameCount);
                    break;
                }
            }

            //设置递增
            i += cbSameCount - 1;
        }

        return;
    }


	bool CShowHandLogic::GetCardTypeData(int iArProCardType[], BYTE cbArCardData[][MAX_COUNT])
	{
		int iArTempProCardType[GAME_PLAYER] = { 0 };
		for (int i = 0; i < GAME_PLAYER; i++)
		{
			iArTempProCardType[i] = iArProCardType[i];
		}

		BYTE cbArTempCardData[GAME_PLAYER][MAX_COUNT] = { 0 };
		memset(cbArTempCardData, 0, sizeof(cbArTempCardData));

		BYTE cbCardListData[MAX_SHOWHAND_POKER] = { 0 };
		memcpy(cbCardListData, m_cbCardListData, sizeof(m_cbCardListData));

		for (int i = 0; i < GAME_PLAYER; i++)
		{
			bool bIsFlag = false;
			if (iArTempProCardType[i] == ShowHand_Pro_Index_TongHuaShun)
			{
				bIsFlag = GetTypeTongHuaShun(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_TieZhi)
			{
				bIsFlag = GetTypeTieZhi(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_HuLu)
			{
				bIsFlag = GetTypeHuLu(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_TongHua) // 
			{
				bIsFlag = GetTypeTongHua(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_ShunZi) // 
			{
				bIsFlag = GetTypeShunZi(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_ThreeTiao)
			{
				bIsFlag = GetTypeThreeTiao(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}			
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_TwoDouble)
			{
				bIsFlag = GetTypeTwoDouble(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_OneDouble)
			{
				bIsFlag = GetTypeOneDouble(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			else if (iArTempProCardType[i] == ShowHand_Pro_Index_Single)
			{
				bIsFlag = GetTypeSingle(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
			}
			//LOG_DEBUG("随机获取牌 1 - i:%d,bIsFlag:%d,cbArTempCardData:0x%02X 0x%02X 0x%02X", i,bIsFlag, cbArTempCardData[i][0], cbArTempCardData[i][1], cbArTempCardData[i][2]);

			// 删除已经获取的牌值
			if (bIsFlag)
			{
				for (int c1 = 0; c1 < MAX_COUNT; c1++)
				{
					BYTE cbCard = cbArTempCardData[i][c1];
					for (int c2 = 0; c2 < MAX_SHOWHAND_POKER; c2++)
					{
						if (cbCard == cbCardListData[c2])
						{
							cbCardListData[c2] = 0;
						}
					}
				}
			}
			else
			{
				iArTempProCardType[i] = ShowHand_Pro_Index_MAX;
				// error
			}

		}

		for (int i = 0; i < GAME_PLAYER; i++)
		{
			if (iArTempProCardType[i] == ShowHand_Pro_Index_MAX)
			{
				bool bIsFlag = GetTypeSingle(cbCardListData, MAX_SHOWHAND_POKER, cbArTempCardData[i], MAX_COUNT);
				//LOG_DEBUG("随机获取牌 2 - i:%d,bIsFlag:%d,cbArTempCardData:0x%02X 0x%02X 0x%02X", i, bIsFlag, cbArTempCardData[i][0], cbArTempCardData[i][1], cbArTempCardData[i][2]);

				// 删除已经获取的牌值
				for (int c1 = 0; c1 < MAX_COUNT; c1++)
				{
					BYTE cbCard = cbArTempCardData[i][c1];
					for (int c2 = 0; c2 < MAX_SHOWHAND_POKER; c2++)
					{
						if (cbCard == cbCardListData[c2])
						{
							cbCardListData[c2] = 0;
						}
					}
				}
			}
		}

		for (int i = 0; i < GAME_PLAYER; i++)
		{
			for (int j = 0; j < MAX_COUNT; j++)
			{
				cbArCardData[i][j] = cbArTempCardData[i][j];
			}
		}
		return false;
	}

	bool CShowHandLogic::IsValidCard(BYTE cbCardData)
	{
		for (int i = 0; i < MAX_SHOWHAND_POKER; i++)
		{
			if (m_cbCardListData[i] == cbCardData)
			{
				return true;
			}
		}
		return false;
	}

	bool CShowHandLogic::GetTypeTongHuaShun(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		BYTE cbTempListCount = 0;
		BYTE cbTempCardListData[MAX_SHOWHAND_POKER] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				cbTempCardListData[cbTempListCount++] = cbCardListData[i];
			}
		}
		for (int i = 0; i < cbTempListCount; i++)
		{
			LOG_DEBUG("获取同花顺 a - i:%d,cbTempListCount:%d,cbTempCardListData:0x%02X", i, cbTempListCount, cbTempCardListData[i]);
		}

		// 统计数值数目
		BYTE cbArCardColorCount[MAX_CARD_VALUE][MAX_CARD_COLOR] = { 0 };
		memset(cbArCardColorCount, 0, sizeof(cbArCardColorCount));
		// 每个值的颜色最多有一张牌
		// 0	1	2	3
		// 
		// 8
		// 9
		// 10
		// 11
		// 12
		// 13
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				cbArCardColorCount[cbCardValue][cbCardColor]++;
			}
		}

		for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
		{
			for (BYTE j = 0; j < MAX_CARD_COLOR; j++)
			{
				LOG_DEBUG("获取同花顺 0 - i:%d,j:%d,cbArCardColorCount:%d",i,j, cbArCardColorCount[i][j]);
			}
		}

		// 找出同花顺
		BYTE cbArAllTargetCard[MAX_SHOWHAND_POKER] = { 0 };
		BYTE cbArAllTargetColor[MAX_SHOWHAND_POKER] = { 0 };
		int iAllTargetCount = 0;
		for (BYTE i = 1; i < MAX_CARD_VALUE - 4; i++)
		{
			//	  1	   8    9   10	 11	  12   13
			// 0x01,0x08,0x09,0x0A,0x0B,0x0C,0x0D
			for (BYTE j = 0; j < MAX_CARD_COLOR; j++)
			{
				if (cbArCardColorCount[i][j] == 1 && cbArCardColorCount[i + 1][j] == 1 && 
					cbArCardColorCount[i + 2][j] == 1 && cbArCardColorCount[i + 3][j] == 1 && cbArCardColorCount[i + 4][j] == 1)
				{
					cbArAllTargetCard[iAllTargetCount] = i;
					cbArAllTargetColor[iAllTargetCount] = j;
					iAllTargetCount++;
				}
			}
		}

		for (int i = 0; i < MAX_SHOWHAND_POKER; i++)
		{
			LOG_DEBUG("获取同花顺 1 - i:%d,iAllTargetCount:%d,cbArAllTargetColor[i]:%d,cbArAllTargetCard[i]:%d",i, iAllTargetCount,cbArAllTargetColor[i], cbArAllTargetCard[i]);
		}

		if (iAllTargetCount <= 0)
		{
			return false;
		}

		// 随机同花顺
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardColor = cbArAllTargetColor[cbTargetPosition];
		BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];


		LOG_DEBUG("获取同花顺 2 - cbTargetPosition:%d,cbTargetCardColor:%d, cbTargetCardValue:%d",	cbTargetPosition, cbTargetCardColor, cbTargetCardValue );


		// 同花顺颜色
		BYTE cbTargetCardCount = 0;
		BYTE cbTargetCard[MAX_COUNT] = { 0 };

		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);

				bool bIsAdd = ((cbCardValue == cbTargetCardValue) || (cbCardValue == cbTargetCardValue + 1) || (cbCardValue == cbTargetCardValue + 2) || (cbCardValue == cbTargetCardValue + 3) || (cbCardValue == cbTargetCardValue + 4));
				if (cbTargetCardCount < MAX_COUNT && cbCardColor == cbTargetCardColor && bIsAdd)
				{
					LOG_DEBUG("获取同花顺 3 - i:%d,cbTargetCardCount:%d,cbCardColor:%d,cbCardValue:%d,cbCardListData:0x%02X", i, cbTargetCardCount, cbCardColor, cbCardValue, cbCardListData[i]);

					cbTargetCard[cbTargetCardCount++] = cbCardListData[i];
				}
			}
		}
		if (cbTargetCardCount != cbCardCount)
		{
			return false;
		}
		for (BYTE i = 0; i < cbCardCount; i++)
		{
			cbCardData[i] = cbTargetCard[i];
		}
		RandCardListEx(cbCardData, cbCardCount);
		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}
		return true;
	}

	bool CShowHandLogic::GetTypeTieZhi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计数值数目
		BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue < MAX_CARD_VALUE)
				{
					cbArCardValueCount[cbCardValue]++;
				}
			}
		}
		// 找出四个
		BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
		{
			if (cbArCardValueCount[i] == 4)
			{
				cbArAllTargetCard[iAllTargetCount] = i;
				iAllTargetCount++;
			}
		}
		if (iAllTargetCount <= 0)
		{
			return false;
		}
		
		// 随机四个
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

		BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };
		BYTE cbTargetCardCount = 0;
		bool bIsAddRand = false;
		BYTE cbCardCountIndex = 0;
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue == cbTargetCardValue)
				{
					cbTargetCard[cbTargetCardCount] = cbCardListData[i];
					cbTargetCardCount++;
				}
				else
				{
					if (bIsAddRand == false)
					{
						cbCardData[cbCardCountIndex++] = cbCardListData[i];
						bIsAddRand = true;
					}
				}
			}
		}
		for (BYTE i = 0; i < cbTargetCardCount; i++)
		{
			cbCardData[cbCardCountIndex++] = cbTargetCard[i];
		}
		RandCardListEx(cbCardData, cbCardCount);
		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}

		return true;
	}
	bool CShowHandLogic::GetTypeHuLu(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计数值数目
		BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue < MAX_CARD_VALUE)
				{
					cbArCardValueCount[cbCardValue]++;
				}
			}
		}
		// 找出三个
		BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllTargetCount = 0;
		BYTE cbArAllSubTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllSubTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
		{
			if (cbArCardValueCount[i] >= 3)
			{
				cbArAllTargetCard[iAllTargetCount] = i;
				iAllTargetCount++;
			}
			if (cbArCardValueCount[i] >= 2)
			{
				cbArAllSubTargetCard[iAllSubTargetCount] = i;
				iAllSubTargetCount++;
			}
		}
		if (iAllTargetCount <= 0 || iAllSubTargetCount <= 0)
		{
			return false;
		}
		// 随机三个
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

		LOG_DEBUG("2 cbTargetPosition:%d,cbTargetCardValue:%d", cbTargetPosition, cbTargetCardValue);
		for (int i = 0; i < iAllTargetCount; i++)
		{
			LOG_DEBUG("3 iAllTargetCount:%d,cbArAllTargetCard:%d", iAllTargetCount, cbArAllTargetCard[i]);
		}

		// 随机两个
		for (BYTE i = 0; i < iAllSubTargetCount; i++)
		{
			if (cbTargetCardValue == cbArAllSubTargetCard[i])
			{
				
				//往前移
				BYTE j = i;
				for (; j < iAllSubTargetCount-1;j++)
				{
					cbArAllSubTargetCard[j] = cbArAllSubTargetCard[j + 1];
				}
				cbArAllSubTargetCard[iAllSubTargetCount - 1] = 0;
				iAllSubTargetCount--;
				break;
			}
		}
		BYTE cbSubTargetPosition = g_RandGen.RandRange(0, iAllSubTargetCount - 1);
		BYTE cbSubTargetCardValue = cbArAllSubTargetCard[cbSubTargetPosition];

		LOG_DEBUG("2 iAllSubTargetCount:%d,cbSubTargetCardValue:%d", cbSubTargetPosition, cbSubTargetCardValue);
		for (int i = 0; i < iAllSubTargetCount; i++)
		{
			LOG_DEBUG("3 iAllTargetCount:%d,cbArAllSubTargetCard:%d", iAllSubTargetCount, cbArAllSubTargetCard[i]);
		}

		BYTE cbTargetCard[MAX_COUNT] = { 0 };
		BYTE cbTargetCardCount = 0;
		BYTE cbSubTargetCardCount = 0;
		BYTE cbCardCountIndex = 0;
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue == cbTargetCardValue && cbTargetCardCount<3)
				{
					cbTargetCard[cbCardCountIndex++] = cbCardListData[i];
					cbTargetCardCount++;
				}
				if (cbCardValue == cbSubTargetCardValue && cbSubTargetCardCount<2)
				{
					cbTargetCard[cbCardCountIndex++] = cbCardListData[i];
					cbSubTargetCardCount++;
				}
			}
		}
		if (cbTargetCardCount + cbSubTargetCardCount != cbCardCount || cbCardCountIndex != cbCardCount)
		{
			return false;
		}
		for (BYTE i = 0; i < cbTargetCardCount + cbSubTargetCardCount; i++)
		{
			cbCardData[i] = cbTargetCard[i];
		}
		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}
		return true;
	}
	bool CShowHandLogic::GetTypeTongHua(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计颜色数目
		BYTE cbArCardColorCount[MAX_CARD_COLOR] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
				if (cbCardColor < MAX_CARD_COLOR)
				{
					cbArCardColorCount[cbCardColor]++;
				}
			}
		}

		// 找出同花
		BYTE cbArAllTarget[MAX_CARD_COLOR] = { 0 };
		int iAllTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_COLOR; i++)
		{
			if (cbArCardColorCount[i] >= 5)
			{
				cbArAllTarget[iAllTargetCount++] = i;
			}
		}
		LOG_DEBUG("1 iAllTargetCount:%d", iAllTargetCount);

		if (iAllTargetCount <= 0)
		{
			return false;
		}

		// 随机颜色
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardColor = cbArAllTarget[cbTargetPosition];

		LOG_DEBUG("2 cbTargetPosition:%d,cbTargetCardColor:%d", cbTargetPosition, cbTargetCardColor);
		for (int i = 0; i < iAllTargetCount; i++)
		{
			LOG_DEBUG("3 iAllTargetCount:%d,cbArAllTarget:%d", iAllTargetCount, cbArAllTarget[i]);
		}

		// 相同颜色的所有牌
		BYTE cbTargetCard[MAX_CARD_VALUE] = { 0 };
		BYTE cbTargetCardCount = 0;
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
				if (cbCardColor == cbTargetCardColor && cbTargetCardCount < MAX_CARD_VALUE)
				{
					cbTargetCard[cbTargetCardCount++] = cbCardListData[i];
				}
			}
		}

		// 选其中五个
		BYTE cbLastTargetCard[MAX_COUNT] = { 0 };
		BYTE cbRandCount = 0, cbPosition = 0;
		do
		{
			BYTE cbPosition = g_RandGen.RandRange(0, cbTargetCardCount - cbRandCount - 1);
			cbLastTargetCard[cbRandCount++] = cbTargetCard[cbPosition];
			cbTargetCard[cbPosition] = cbTargetCard[cbTargetCardCount - cbRandCount];
		} while (cbRandCount < cbCardCount);

		LOG_DEBUG("cbRandCount:%d,cbLastTargetCard:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
			cbRandCount, cbLastTargetCard[0], cbLastTargetCard[1], cbLastTargetCard[2], cbLastTargetCard[3], cbLastTargetCard[4]);


		//检查是否同花
		SortCardList(cbLastTargetCard, MAX_COUNT);
		bool bLineCard = true;
		BYTE cbFirstValue = GetCardValue(cbLastTargetCard[0]);
		for (BYTE i = 1; i<cbCardCount; i++)
		{
			if (cbFirstValue != (GetCardValue(cbLastTargetCard[i]) + i)) bLineCard = false;
			if (bLineCard == false)break;
		}

		if (bLineCard == false)
		{
			for (BYTE i = 0; i < cbCardCount; i++)
			{
				cbCardData[i] = cbLastTargetCard[i];
			}
			for (int i = 0; i < cbCardCount; i++)
			{
				if (IsValidCard(cbCardData[i]) == false)
				{
					return false;
				}
			}
			return true;
		}

		return false;
	}
	bool CShowHandLogic::GetTypeShunZi(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计数值数目
		BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue < MAX_CARD_VALUE)
				{
					cbArCardValueCount[cbCardValue]++;
				}
			}
		}

		// 找出顺子
		BYTE cbArAllTargetData[MAX_CARD_VALUE] = { 0 };
		int iAllTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_VALUE - 4; i++)
		{
			//	  0	   1	2    3	  4	   5	6	 7	  8	   9   10	11	 12	  13
			// 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D };
			// 需要有两种花色才不会随到顺金
			if (cbArCardValueCount[i] >= 2 && cbArCardValueCount[i + 1] >= 2 && cbArCardValueCount[i + 2] >= 2 && cbArCardValueCount[i + 3] >= 2 && cbArCardValueCount[i + 4] >= 2)
			{
				cbArAllTargetData[iAllTargetCount++] = i;
			}
		}
		if (iAllTargetCount <= 0)
		{
			return false;
		}
		// 随机顺子
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardValue = cbArAllTargetData[cbTargetPosition];

		// 选出该数值顺子的全部扑克
		BYTE cbTargetColorCount[MAX_COUNT] = { 0 };
		BYTE cbTargetCard[MAX_COUNT][MAX_CARD_COLOR] = { 0 };
		// 三行四列
		// 06 16 26 36
		// 07 17
		// 08 28 38	
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardColor = GetCardColorValue(cbCardListData[i]);
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				for (BYTE j = 0; j < MAX_COUNT; j++)
				{
					if (cbCardValue == cbTargetCardValue + j)
					{
						if (cbTargetColorCount[j] < MAX_CARD_COLOR)
						{
							cbTargetCard[j][cbTargetColorCount[j]] = cbCardListData[i];
							cbTargetColorCount[j]++;
						}
					}
				}
			}
		}

		// 取出顺子
		bool bIsHaveCard = true;
		for (BYTE i = 0; i < MAX_COUNT; i++)
		{
			if (cbTargetColorCount[i] == 0)
			{
				bIsHaveCard = false;
			}
		}
		if (bIsHaveCard)
		{
			for (BYTE i = 0; i < MAX_COUNT; i++)
			{
				if (cbTargetColorCount[i] > 0)
				{
					BYTE cbPosition = g_RandGen.RandRange(0, cbTargetColorCount[i] - 1);
					cbCardData[i] = cbTargetCard[i][cbPosition];
				}
				else
				{
					return false;
				}
			}

			// 需要有两种花色才不会随到顺金
			bool bIsYiSe = true;
			BYTE cbCurColor = 0xFF;
			for (BYTE i = 0; i < MAX_COUNT; i++)
			{
				BYTE cbCardColor = GetCardColorValue(cbCardData[i]);
				if (cbCurColor == 0xFF)
				{
					cbCurColor = cbCardColor;
				}
				if (cbCurColor != cbCardColor)
				{
					bIsYiSe = false;
				}
			}
			if (bIsYiSe)
			{
				bool bIsBreak = false;
				for (BYTE i = 0; i < MAX_COUNT; i++)
				{
					BYTE cbCardColor = GetCardColorValue(cbCardData[i]);
					for (BYTE j = 0; j < cbTargetColorCount[i]; j++)
					{
						if (cbCardColor != GetCardColorValue(cbTargetCard[i][j]))
						{
							cbCardData[i] = cbTargetCard[i][j];
							bIsBreak = true;
							break;
							//return true;
						}
					}
					if (bIsBreak)
					{
						break;
					}
				}
			}

			for (int i = 0; i < cbCardCount; i++)
			{
				if (IsValidCard(cbCardData[i]) == false)
				{
					return false;
				}
			}

			return true;
		}
		return true;
	}
	bool CShowHandLogic::GetTypeThreeTiao(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计数值数目
		BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue < MAX_CARD_VALUE)
				{
					cbArCardValueCount[cbCardValue]++;
				}
			}
		}
		// 找出三个
		BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
		{
			if (cbArCardValueCount[i] >= 3)
			{
				cbArAllTargetCard[iAllTargetCount] = i;
				iAllTargetCount++;
			}
		}
		if (iAllTargetCount <= 0)
		{
			return false;
		}

		// 随机三个
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

		BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };
		BYTE cbTargetCardCount = 0;
		BYTE cbFirstCardValue = 0xFF;
		BYTE cbCardCountIndex = 0;
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue == cbTargetCardValue && cbTargetCardCount < 3)
				{
					cbTargetCard[cbTargetCardCount] = cbCardListData[i];
					cbTargetCardCount++;
				}
				else if (cbCardCountIndex < 2)
				{
					if (cbFirstCardValue == 0xFF)
					{
						cbCardData[cbCardCountIndex++] = cbCardListData[i];
						cbFirstCardValue = cbCardValue;
					}
					else if (cbFirstCardValue != cbCardValue)
					{
						cbCardData[cbCardCountIndex++] = cbCardListData[i];
					}
				}
			}
		}

		if (cbTargetCardCount + cbCardCountIndex != cbCardCount)
		{
			return false;
		}

		for (BYTE i = 0; i < cbTargetCardCount; i++)
		{
			cbCardData[cbCardCountIndex++] = cbTargetCard[i];
		}
		RandCardListEx(cbCardData, cbCardCount);
		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}

		return true;
	}
	bool CShowHandLogic::GetTypeTwoDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计数值数目
		BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue < MAX_CARD_VALUE)
				{
					cbArCardValueCount[cbCardValue]++;
				}
			}
		}
		// 找出三个
		BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllTargetCount = 0;
		BYTE cbArAllSubTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllSubTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
		{
			if (cbArCardValueCount[i] >= 2)
			{
				cbArAllTargetCard[iAllTargetCount] = i;
				iAllTargetCount++;
			}
			if (cbArCardValueCount[i] >= 2)
			{
				cbArAllSubTargetCard[iAllSubTargetCount] = i;
				iAllSubTargetCount++;
			}
		}
		if (iAllTargetCount <= 0 || iAllSubTargetCount <= 0)
		{
			return false;
		}
		for (int i = 0; i < iAllTargetCount; i++)
		{
			LOG_DEBUG("1 iAllTargetCount:%d,cbArAllTargetCard:%d", iAllTargetCount, cbArAllTargetCard[i]);
		}
		for (int i = 0; i < iAllSubTargetCount; i++)
		{
			LOG_DEBUG("1 iAllTargetCount:%d,cbArAllSubTargetCard:%d", iAllSubTargetCount, cbArAllSubTargetCard[i]);
		}
		// 随机两个
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

		// 随机两个
		for (BYTE i = 0; i < iAllSubTargetCount; i++)
		{
			if (cbTargetCardValue == cbArAllSubTargetCard[i])
			{
				//往前移
				BYTE j = i;
				for (; j < iAllSubTargetCount - 1; j++)
				{
					cbArAllSubTargetCard[j] = cbArAllSubTargetCard[j + 1];
				}
				cbArAllSubTargetCard[iAllSubTargetCount - 1] = 0;
				iAllSubTargetCount--;
				break;
			}
		}

		for (int i = 0; i < iAllTargetCount; i++)
		{
			LOG_DEBUG("2 iAllTargetCount:%d,cbArAllTargetCard:%d", iAllTargetCount, cbArAllTargetCard[i]);
		}
		for (int i = 0; i < iAllSubTargetCount; i++)
		{
			LOG_DEBUG("2 iAllTargetCount:%d,cbArAllSubTargetCard:%d", iAllSubTargetCount, cbArAllSubTargetCard[i]);
		}

		BYTE cbSubTargetPosition = g_RandGen.RandRange(0, iAllSubTargetCount - 1);
		BYTE cbSubTargetCardValue = cbArAllSubTargetCard[cbSubTargetPosition];

		LOG_DEBUG("cbTargetPosition:%d,cbTargetCardValue:%d", cbTargetPosition, cbTargetCardValue);
		LOG_DEBUG("cbSubTargetPosition:%d,cbSubTargetCardValue:%d", cbSubTargetPosition, cbSubTargetCardValue);

		BYTE cbTargetCard[MAX_COUNT] = { 0 };
		BYTE cbTargetCardCount = 0;
		BYTE cbSubTargetCardCount = 0;
		BYTE cbCardCountIndex = 0;
		bool bIsAddLastCard = false;
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue == cbTargetCardValue && cbTargetCardCount<2)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					cbTargetCardCount++;
				}
				else if (cbCardValue == cbSubTargetCardValue && cbSubTargetCardCount<2)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					cbSubTargetCardCount++;
				}
				else if(bIsAddLastCard==false)
				{
					cbCardData[cbCardCountIndex++] = cbCardListData[i];
					bIsAddLastCard = true;
				}
			}
		}
		if (cbCardCountIndex != cbCardCount)
		{
			return false;
		}

		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}

		LOG_DEBUG("cbCardCount:%d,cbCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", cbCardCount, cbCardData[0], cbCardData[1], cbCardData[2], cbCardData[3], cbCardData[4]);

		return true;
	}
	bool CShowHandLogic::GetTypeOneDouble(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		// 统计数值数目
		BYTE cbArCardValueCount[MAX_CARD_VALUE] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbCardListData[i]);
				if (cbCardValue < MAX_CARD_VALUE)
				{
					cbArCardValueCount[cbCardValue]++;
				}
			}
		}
		// 找出三个
		BYTE cbArAllTargetCard[MAX_CARD_VALUE] = { 0 };
		int iAllTargetCount = 0;
		for (BYTE i = 0; i < MAX_CARD_VALUE; i++)
		{
			if (cbArCardValueCount[i] >= 2)
			{
				cbArAllTargetCard[iAllTargetCount] = i;
				iAllTargetCount++;
			}
		}
		if (iAllTargetCount <= 0)
		{
			return false;
		}

		// 随机两个
		BYTE cbTargetPosition = g_RandGen.RandRange(0, iAllTargetCount - 1);
		BYTE cbTargetCardValue = cbArAllTargetCard[cbTargetPosition];

		BYTE cbTargetCard[MAX_CARD_COLOR] = { 0 };
		BYTE cbTargetCardCount = 0;
		BYTE cbFirstCardValue = 0xFF;
		BYTE cbSecondCardValue = 0xFF;
		BYTE cbCardCountIndex = 0;

		BYTE cbTempCardListData[MAX_SHOWHAND_POKER] = { 0 };
		memcpy(cbTempCardListData, cbCardListData, sizeof(cbTempCardListData));
		RandCardListEx(cbTempCardListData, cbListCount);


		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbTempCardListData[i]))
			{
				BYTE cbCardValue = GetCardValue(cbTempCardListData[i]);
				if (cbCardValue == cbTargetCardValue)
				{
					if (cbTargetCardCount < 2)
					{
						cbTargetCard[cbTargetCardCount] = cbTempCardListData[i];
						cbTargetCardCount++;
					}
				}
				else if (cbCardCountIndex < 3)
				{
					if (cbFirstCardValue == 0xFF)
					{
						cbCardData[cbCardCountIndex++] = cbTempCardListData[i];
						cbFirstCardValue = cbCardValue;
					}
					else if (cbSecondCardValue == 0xFF && cbFirstCardValue != cbCardValue)
					{
						cbCardData[cbCardCountIndex++] = cbTempCardListData[i];
						cbSecondCardValue = cbCardValue;
					}
					else if(cbFirstCardValue != cbCardValue && cbSecondCardValue != cbCardValue)
					{
						cbCardData[cbCardCountIndex++] = cbTempCardListData[i];
					}
				}
			}
		}

		//LOG_DEBUG("cbTargetCardCount:%d,cbCardCountIndex:%d,cbCardCount:%d,cbFirstCardValue:%d,cbSecondCardValue:%d",
			//cbTargetCardCount,cbCardCountIndex, cbCardCount, cbFirstCardValue, cbSecondCardValue);

		if (cbTargetCardCount + cbCardCountIndex != cbCardCount)
		{
			return false;
		}

		for (BYTE i = 0; i < cbTargetCardCount; i++)
		{
			cbCardData[cbCardCountIndex++] = cbTargetCard[i];
		}

		//LOG_DEBUG("cbCardCount:%d,cbCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", cbCardCount, cbCardData[0], cbCardData[1], cbCardData[2], cbCardData[3], cbCardData[4]);

		RandCardListEx(cbCardData, cbCardCount);

		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				return false;
			}
		}

		return true;
	}
	//获取散牌
	bool CShowHandLogic::GetTypeSingle(BYTE cbCardListData[], BYTE cbListCount, BYTE cbCardData[], BYTE cbCardCount)
	{
		BYTE cbTempListCount = 0;
		BYTE cbArTempCardData[MAX_SHOWHAND_POKER] = { 0 };
		for (BYTE i = 0; i < cbListCount; i++)
		{
			if (IsValidCard(cbCardListData[i]))
			{
				cbArTempCardData[cbTempListCount++] = cbCardListData[i];
			}
		}
		if (cbTempListCount < cbCardCount)
		{
			LOG_DEBUG("cbTempListCount:%d,cbCardCount:%d", cbTempListCount, cbCardCount);
			return false;
		}
		// 随便选三张牌 不用管了
		BYTE cbTargetCard[MAX_COUNT] = { 0 };
		for (int i = 0; i < 1000; i++)
		{
			memset(cbTargetCard, 0, sizeof(cbTargetCard));
			BYTE cbTempCardListData[MAX_SHOWHAND_POKER] = { 0 };
			memcpy(cbTempCardListData, cbArTempCardData, sizeof(cbTempCardListData));
			BYTE cbRandCount = 0, cbPosition = 0;
			do
			{
				BYTE cbPosition = g_RandGen.RandRange(0, cbTempListCount - cbRandCount - 1);
				cbTargetCard[cbRandCount++] = cbTempCardListData[cbPosition];
				cbTempCardListData[cbPosition] = cbTempCardListData[cbTempListCount - cbRandCount];
			} while (cbRandCount < cbCardCount);

			if (GetCardGenre(cbTargetCard, cbCardCount) == CT_SINGLE)
			{
				break;
			}
		}

		for (BYTE i = 0; i < cbCardCount; i++)
		{
			cbCardData[i] = cbTargetCard[i];
		}

		for (int i = 0; i < cbCardCount; i++)
		{
			if (IsValidCard(cbCardData[i]) == false)
			{
				LOG_DEBUG("i:%d,cbCardCount:%d,cbCardData[i]:0x%02X", i, cbCardCount, cbCardData[i]);
				return false;
			}
		}

		//LOG_DEBUG("cbCardCount:%d,cbCardData:0x%02X 0x%02X 0x%02X 0x%02X 0x%02X", cbCardCount, cbCardData[0], cbCardData[1], cbCardData[2], cbCardData[3], cbCardData[4]);

		return true;
	}

};
