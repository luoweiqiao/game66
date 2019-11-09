

#include "majiang_logic.h"
#include "math/rand_table.h"
#include "majiang_config.h"
#include "game_imple_table.h"
#include <center_log.h>

using namespace game_majiang;
//静态变量
namespace game_majiang {


    /*
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,						//万子
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,						//索子
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,						//同子
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,									//番子
    0x38 // 无效牌
    */

	//构造函数
	CMaJiangLogic::CMaJiangLogic(){
		m_kingCardIndexs.clear();
		m_pMjCfg = NULL;
		m_pHostTable = NULL;
		m_bIsAnalyseTingCard15 = false;
	}

	//析构函数
	CMaJiangLogic::~CMaJiangLogic() {
	}
	void CMaJiangLogic::SetMjCfg(CMaJiangCfg* pCfg){
		m_pMjCfg = pCfg;
	}
	void CMaJiangLogic::SetMjTable(CGameMaJiangTable* pTable){
		m_pHostTable = pTable;
	}

	//获取动作名称
	string  CMaJiangLogic::GetOperCodeName(uint32 operCode)
	{
		//static string name[]={"NULL","左吃","中吃","右吃","碰","杠","听","胡","起手胡","要海底","杠听"};
		//if(operCode>ACTION_GANG_TING)
		//	return "";
		//return name[operCode];
		return "";
	}
//获取牌的名称
	string  CMaJiangLogic::GetCardName(BYTE card)
	{
		BYTE color=GetCardColorValue(card);
		BYTE value=GetCardValue(card);

		if(color==emPOKER_COLOR_WAN){
			return CStringUtility::FormatToString("%d万",value);
		}else if(color==emPOKER_COLOR_SUO){
			return CStringUtility::FormatToString("%d条",value);
		}else if(color==emPOKER_COLOR_TONG){
			return CStringUtility::FormatToString("%d万",value);
		}else{
			return CStringUtility::FormatToString("%d字",value);
		}
		return "";
	}
	//打印牌型
	string  CMaJiangLogic::PrintHuType(const stAnalyseItem& item) {
		stringstream ss;
		ss << "将:" << GetCardName(item.cbCardEye);
		for (int32 i = 0; i < MAX_WEAVE; ++i) {
			if (item.WeaveKind[i] == ACTION_GANG) {
				ss << " 杠:" << GetCardName(item.cbCenterCard[i]);
			} else if (item.WeaveKind[i] == ACTION_PENG) {
				ss << " 碰:" << GetCardName(item.cbCenterCard[i]);
			} else if (item.WeaveKind[i] == ACTION_EAT_LEFT) {
				ss << " 吃:" << GetCardName(item.cbCenterCard[i]) << GetCardName(item.cbCenterCard[i] + 1) <<
				GetCardName(item.cbCenterCard[i] + 2);
			} else if (item.WeaveKind[i] == ACTION_EAT_CENTER) {
				ss << " 吃:" << GetCardName(item.cbCenterCard[i] - 1) << GetCardName(item.cbCenterCard[i]) <<
				GetCardName(item.cbCenterCard[i] + 1);
			} else if (item.WeaveKind[i] == ACTION_EAT_RIGHT) {
				ss << " 吃:" << GetCardName(item.cbCenterCard[i] - 2) << GetCardName(item.cbCenterCard[i] - 1) <<
				GetCardName(item.cbCenterCard[i]);
			}
		}
		return ss.str();
	}
	//打印起手胡
	string  CMaJiangLogic::PrintOpenHuType(stActionOper& oper)
	{
		static string name[]={"","小四喜","板板胡","缺一色","六六顺","节节高","三同","一枝花","金童玉女","一点红"};
		stringstream ss;
		for(uint8 i=1;i<MINHU_MAX_TYPE;++i){
			if(oper.isExist(i)){
				ss<<" "<<name[i];
			}
		}
		return ss.str();
	}
	//打印大胡
	string  CMaJiangLogic::PrintHuKindType(stActionOper& oper)
	{
		static string name[]={"","基本胡","七对","13乱","将将胡","起手4红中","碰碰胡","清一色","豪华七小对","全求人","门清","双豪华七小对",
				"大四喜","大三元","四杠","连七对","百万石","小四喜","小三元","字一色","四暗刻",	"一色双龙会",	"一色四同顺",	"一色四节高",
				"一色四步高","三杠","混幺九","一色三同顺","一色三节高","清龙",	"一色三步高",	"三暗刻","天听","大于五","小于五","三风刻",
				"妙手回春","混一色","双暗杠","全带幺","不求人","双明杠","胡绝张","立直","箭刻","圈风刻",	"门风刻","四归一","双暗刻",
				"暗杠","断幺九","二五八将","幺九头","报听","一般高","连六","老少副","幺九刻","明杠","边张","坎张","单调将","双箭刻"};
		stringstream ss;
		for(uint8 i=1;i<HUTYPE_MAX_TYPE;++i){
			if(oper.isExist(i)){
				ss<<" "<<name[i];
			}
		}
		return ss.str();
	}
	//打印权位
	string  CMaJiangLogic::PrintHuRightType(uint32 huRight)
	{
		static string name[]={"","自摸","抢杠胡","杠上开花","天胡","地胡","杠上炮","海底捞月","抢海底"};
		stringstream ss;
		for(uint8 i=1;i<=8;++i){
			if(huRight&(1<<i)){
				ss<<" "<<name[i];
			}
		}
		return ss.str();
	}
	//删除扑克
	bool CMaJiangLogic::RemoveCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbRemoveCard)
	{
		//效验扑克
		ASSERT(IsValidCard(cbRemoveCard));
		ASSERT(cbCardIndex[SwitchToCardIndex(cbRemoveCard)]>0);

		//删除扑克
		BYTE cbRemoveIndex=SwitchToCardIndex(cbRemoveCard);
		if (cbCardIndex[cbRemoveIndex]>0)
		{
			cbCardIndex[cbRemoveIndex]--;
			return true;
		}

		//失败效验
		ASSERT(FALSE);

		return false;
	}

    //删除扑克
	bool CMaJiangLogic::RemoveCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbRemoveCard[], BYTE cbRemoveCount)
	{
		//删除扑克
		for (BYTE i=0;i<cbRemoveCount;i++)
		{
			//效验扑克
			ASSERT(IsValidCard(cbRemoveCard[i]));
			ASSERT(cbCardIndex[SwitchToCardIndex(cbRemoveCard[i])]>0);

			//删除扑克
			BYTE cbRemoveIndex=SwitchToCardIndex(cbRemoveCard[i]);
			if (cbCardIndex[cbRemoveIndex]==0)
			{
				//错误断言
				ASSERT(FALSE);

				//还原删除
				for (BYTE j=0;j<i;j++)
				{
					ASSERT(IsValidCard(cbRemoveCard[j]));
					cbCardIndex[SwitchToCardIndex(cbRemoveCard[j])]++;
				}

				return false;
			}
			else
			{
				//删除扑克
				--cbCardIndex[cbRemoveIndex];
			}
		}

		return true;
	}

	//删除扑克
	bool CMaJiangLogic::RemoveCard(BYTE cbCardData[], BYTE cbCardCount, BYTE cbRemoveCard[], BYTE cbRemoveCount)
	{
		//检验数据
		ASSERT(cbCardCount<=MAX_COUNT);
		ASSERT(cbRemoveCount<=cbCardCount);

		//定义变量
		BYTE cbDeleteCount=0,cbTempCardData[MAX_COUNT];
		if (cbCardCount>getArrayLen(cbTempCardData))	return false;

		memcpy(cbTempCardData,cbCardData,cbCardCount*sizeof(cbCardData[0]));

		//置零扑克
		for (BYTE i=0;i<cbRemoveCount;i++)
		{
			for (BYTE j=0;j<cbCardCount;j++)
			{
				if (cbRemoveCard[i]==cbTempCardData[j])
				{
					cbDeleteCount++;
					cbTempCardData[j]=0;
					break;
				}
			}
		}

		//成功判断
		if (cbDeleteCount!=cbRemoveCount)
		{
			ASSERT(FALSE);
			return false;
		}

		//清理扑克
		BYTE cbCardPos=0;
		for (BYTE i=0;i<cbCardCount;i++)
		{
			if (cbTempCardData[i]!=0)
				cbCardData[cbCardPos++]=cbTempCardData[i];
		}

		return true;
	}
    //设置精牌
    void CMaJiangLogic::SetKingCardIndex(BYTE cbKingCardIndex)
    {
		m_kingCardIndexs.clear();
		m_kingCardIndexs.push_back(cbKingCardIndex);
    }
    //判断精牌
    bool CMaJiangLogic::IsKingCardData(BYTE cbCardData)
    {
		if(m_kingCardIndexs.size()==0)return false;

        return m_kingCardIndexs[0]==SwitchToCardIndex(cbCardData);
    }
    //判断精牌
    bool CMaJiangLogic::IsKingCardIndex(BYTE cbCardIndex)
    {
		if(m_kingCardIndexs.size()==0)return false;

        return m_kingCardIndexs[0]==cbCardIndex;
    }
	//有效判断
	bool CMaJiangLogic::IsValidCard(BYTE cbCardData)
	{
		BYTE cbValue=(cbCardData&MASK_VALUE);
		BYTE cbColor=(cbCardData&MASK_COLOR)>>4;
		return (((cbValue>=1)&&(cbValue<=9)&&(cbColor<=2))||((cbValue>=1)&&(cbValue<=7)&&(cbColor==3)));
		//if (cbCardData>= emPOKER_WAN1 && emPOKER_BAI<= cbCardData)
		//{
		//	return true;
		//}
		//return false;
	}

	//扑克数目
	BYTE CMaJiangLogic::GetCardCount(BYTE cbCardIndex[MAX_INDEX])
	{
		//数目统计
		BYTE cbCardCount=0;
		for (BYTE i=0;i<MAX_INDEX;i++)
			cbCardCount+=cbCardIndex[i];

		return cbCardCount;
	}

	//获取组合
	BYTE CMaJiangLogic::GetWeaveCard(BYTE cbWeaveKind, BYTE cbCenterCard, BYTE cbCardBuffer[4])
	{
		//组合扑克
		switch (cbWeaveKind)
		{
			case ACTION_EAT_LEFT:		//左吃操作
			{
				//设置变量
				cbCardBuffer[0]=cbCenterCard+1;
				cbCardBuffer[1]=cbCenterCard+2;
				cbCardBuffer[2]=cbCenterCard;

				return 3;
			}
			case ACTION_EAT_RIGHT:		//右吃操作
			{
				//设置变量
				cbCardBuffer[0]=cbCenterCard-2;
				cbCardBuffer[1]=cbCenterCard-1;
				cbCardBuffer[2]=cbCenterCard;

				return 3;
			}
			case ACTION_EAT_CENTER:		//中吃操作
			{
				//设置变量
				cbCardBuffer[0]=cbCenterCard-1;
				cbCardBuffer[1]=cbCenterCard;
				cbCardBuffer[2]=cbCenterCard+1;

				return 3;
			}
			case ACTION_PENG:		//碰牌操作
			{
				//设置变量
				cbCardBuffer[0]=cbCenterCard;
				cbCardBuffer[1]=cbCenterCard;
				cbCardBuffer[2]=cbCenterCard;

				return 3;
			}
			case ACTION_GANG:		//杠牌操作
			{
				//设置变量
				cbCardBuffer[0]=cbCenterCard;
				cbCardBuffer[1]=cbCenterCard;
				cbCardBuffer[2]=cbCenterCard;
				cbCardBuffer[3]=cbCenterCard;

				return 4;
			}
			default:
			{
				ASSERT(FALSE);
			}
		}
		return 0;
	}

	//动作等级
	BYTE CMaJiangLogic::GetUserActionRank(DWORD UserAction)
	{
        //起手胡
        if(UserAction&ACTION_QIPAI_HU){ return 5; }
		//胡牌等级
		if(m_pMjCfg->supportQiangGang()){
			if(UserAction & ACTION_CHI_HU){ return 3; }// 胡降级跟杠一样,抢杠胡操作
		}else{
			if(UserAction & ACTION_CHI_HU){ return 4; }
		}
		//杠牌等级
		if(UserAction&ACTION_GANG_TING){ return 3; }
		if(UserAction&ACTION_GANG){ return 3; }
		//碰牌等级
		if(UserAction&ACTION_PENG) { return 2; }
		//上牌等级
		if(UserAction&(ACTION_EAT_RIGHT|ACTION_EAT_CENTER|ACTION_EAT_LEFT)) { return 1; }

		return 0;
	}
	//取出动作数组
	void CMaJiangLogic::GetAllAction(DWORD actionMask,vector<BYTE>& actions)
	{
		actions.clear();
		for(uint8 i=0;i<31;++i)
		{
			uint32 mask=1<<i;
			if(actionMask&mask){
				actions.push_back(i);
			}
		}
	}
	//取出一个动作
	BYTE  CMaJiangLogic::GetOneAction(DWORD actionMask)
	{
		for(uint8 i=0;i<31;++i){
			uint32 mask=1<<i;
			if(actionMask&mask){
				return i;
			}
		}
		return 0;
	}
	BYTE  CMaJiangLogic::GetOneAIAction(DWORD actionMask, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, BYTE cbCurrentCard, uint16 curChairID, BYTE cbListenStatus[GAME_PLAYER])
	{
		vector<BYTE> actions;
		actions.clear();
		for (uint8 i = 0; i < 31; ++i)
		{
			uint32 mask = 1 << i;
			if (actionMask&mask)
			{
				actions.push_back(i);
			}
		}

		vector<BYTE>::iterator iter_hu = find(actions.begin(), actions.end(), 7);
		if (iter_hu != actions.end())
		{
			return 7;
		}
		vector<BYTE>::iterator iter_ting = find(actions.begin(), actions.end(), 6);
		if (iter_ting != actions.end())
		{
			return 6;
		}
		vector<BYTE>::iterator iter_gang = find(actions.begin(), actions.end(), 5);
		if (iter_gang != actions.end() && IsValidCard(cbCurrentCard))
		{
			if (cbCurrentCard >= emPOKER_DONG && emPOKER_BAI <= cbCurrentCard)
			{
				return 5;
			}

			int iCardCount = GetCardCount(cbCardIndex);

			LOG_DEBUG("gang_card - roomid:%d,tableid:%d,uid:%d - %d,cbCurrentCard:0x%02X,iCardCount:%d,GetPlayerCount:%d,curChairID:%d,cbListenStatus:%d - %d",
				m_pHostTable->GetRoomID(), m_pHostTable->GetTableID(), m_pHostTable->GetPlayerID(0), m_pHostTable->GetPlayerID(1), cbCurrentCard,iCardCount,m_pHostTable->GetPlayerCount(), curChairID, cbListenStatus[0], cbListenStatus[1]);

			if (iCardCount == 14 || iCardCount == 11 || iCardCount == 8 || iCardCount == 5)// 暗杠
			{
				return 5;
			}
			else // 明杠
			{
				bool bOtherIsTingCard = false;
				for (uint32 i = 0; i < m_pHostTable->GetPlayerCount(); i++)
				{
					if (i == curChairID)
					{
						continue;
					}
					if (cbListenStatus[i]==TRUE)
					{
						bOtherIsTingCard = true;
					}
				}

				BYTE cbHandCardIndex[MAX_INDEX] = { 0 };
				for (uint32 j = 0; j < MAX_INDEX; j++)
				{
					cbHandCardIndex[j] = cbCardIndex[j];
				}
				BYTE cbCurrentIndex = SwitchToCardIndex(cbCurrentCard);
				cbHandCardIndex[cbCurrentIndex] ++;
				map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
				DWORD dwAction = AnalyseTingCard17(curChairID, cbHandCardIndex, WeaveItem, cbWeaveCount, mpTingNotifyHu);

				LOG_DEBUG("gang_card - roomid:%d,tableid:%d,uid:%d - %d,cbCurrentCard:0x%02X,iCardCount:%d,GetPlayerCount:%d,curChairID:%d,cbListenStatus:%d - %d,dwAction:%d,mpTingNotifyHu.size:%d,bOtherIsTingCard:%d",
					m_pHostTable->GetRoomID(), m_pHostTable->GetTableID(), m_pHostTable->GetPlayerID(0), m_pHostTable->GetPlayerID(1), cbCurrentCard, iCardCount, m_pHostTable->GetPlayerCount(), curChairID, cbListenStatus[0], cbListenStatus[1], dwAction, mpTingNotifyHu.size(), bOtherIsTingCard);

				if (dwAction == ACTION_LISTEN && mpTingNotifyHu.size()>0)
				{
					return 5;
				}
				if (bOtherIsTingCard)
				{
					actions.erase(iter_gang);
				}
				else
				{
					return 5;
				}
			}
		}
		vector<BYTE>::iterator iter_peng = find(actions.begin(), actions.end(), 4);
		if (iter_peng != actions.end() && IsValidCard(cbCurrentCard))
		{
			BYTE cbCurrentIndex = SwitchToCardIndex(cbCurrentCard);
			if (cbCurrentCard >= emPOKER_DONG && emPOKER_BAI <= cbCurrentCard)
			{
				return 4;
			}
			bool bIsPengCard = true;

			if (cbCurrentIndex == 0 && cbCardIndex[cbCurrentIndex + 1] != 0)
			{
				bIsPengCard = false;
				actions.erase(iter_peng);
			}
			if (cbCurrentIndex == 8 && cbCardIndex[cbCurrentIndex - 1] != 0)
			{
				bIsPengCard = false;
				actions.erase(iter_peng);
			}
			if (cbCurrentIndex != 0 && cbCurrentIndex != 8 && (cbCardIndex[cbCurrentIndex - 1] != 0 || cbCardIndex[cbCurrentIndex + 1] != 0))
			{
				bIsPengCard = false;
				actions.erase(iter_peng);
			}
			// 碰了之后能听牌就要碰
			BYTE cbHandCardIndex[MAX_INDEX] = { 0 };
			for (uint32 j = 0; j < MAX_INDEX; j++)
			{
				cbHandCardIndex[j] = cbCardIndex[j];
			}
			cbHandCardIndex[cbCurrentIndex] ++;
			map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
			DWORD dwAction = AnalyseTingCard17(curChairID,cbHandCardIndex, WeaveItem, cbWeaveCount, mpTingNotifyHu);
			if (dwAction == ACTION_LISTEN && mpTingNotifyHu.size()>0)
			{
				bIsPengCard = true;
			}
			if (bIsPengCard == true)
			{
				return 4;
			}
		}
		//vector<BYTE>::iterator iter_peng = find(actions.begin(), actions.end(), 4);
		//if (iter_peng != actions.end() && iter_gang != actions.end())
		//{
		//	return 5;
		//}
		// 1 2 3
		if (IsValidCard(cbCurrentCard))
		{
			BYTE cbCurrentIndex = SwitchToCardIndex(cbCurrentCard);
			BYTE cbHandCardIndex[MAX_INDEX] = { 0 };
			for (uint32 j = 0; j < MAX_INDEX; j++)
			{
				cbHandCardIndex[j] = cbCardIndex[j];
			}
			cbHandCardIndex[cbCurrentIndex] ++;

			vector<BYTE>::iterator iter_eat_left = find(actions.begin(), actions.end(), 1);
			if (iter_eat_left != actions.end() && cbCurrentIndex <= 6)
			{
				bool bIsEat_Flag = true;
				if (cbCardIndex[cbCurrentIndex] != 0)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex] == 1 && cbCardIndex[cbCurrentIndex + 1] == 1 && cbCardIndex[cbCurrentIndex + 2] == 1)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex + 1] >= 2 || cbCardIndex[cbCurrentIndex + 2] >= 2)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex + 1] != 0 && cbCardIndex[cbCurrentIndex + 2] != 0 && cbCardIndex[cbCurrentIndex + 3] != 0)
				{
					bIsEat_Flag = false;
				}
				// 吃了之后能听牌就要吃
				map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
				DWORD dwAction = AnalyseTingCard17(curChairID, cbHandCardIndex, WeaveItem, cbWeaveCount, mpTingNotifyHu);
				if (dwAction == ACTION_LISTEN && mpTingNotifyHu.size()>0)
				{
					bIsEat_Flag = true;
				}
				if (bIsEat_Flag == false)
				{
					actions.erase(iter_eat_left);
				}
			}

			vector<BYTE>::iterator iter_eat_center = find(actions.begin(), actions.end(), 2);
			if (iter_eat_center != actions.end() && cbCurrentIndex != 0 && cbCurrentIndex != 8 && cbCurrentIndex < 9)
			{
				bool bIsEat_Flag = true;
				if (cbCardIndex[cbCurrentIndex] != 0)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 1] == 1 && cbCardIndex[cbCurrentIndex] == 1 && cbCardIndex[cbCurrentIndex + 1] == 1)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 1] >= 2 && cbCardIndex[cbCurrentIndex + 1] >= 2)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 1] >= 2 && cbCardIndex[cbCurrentIndex + 1] >= 1)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 1] >= 1 && cbCardIndex[cbCurrentIndex + 1] >= 2)
				{
					bIsEat_Flag = false;
				}
				// 吃了之后能听牌就要吃
				map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
				DWORD dwAction = AnalyseTingCard17(curChairID, cbHandCardIndex, WeaveItem, cbWeaveCount, mpTingNotifyHu);
				if (dwAction == ACTION_LISTEN && mpTingNotifyHu.size()>0)
				{
					bIsEat_Flag = true;
				}

				LOG_DEBUG("eat_center - roomid:%d,tableid:%d,uid:%d - %d,cbCurrentCard:0x%02X,cbCurrentIndex:%d,curChairID:%d,cbCardIndex:%d - %d,dwAction:%d,bIsEat_Flag:%d",
					m_pHostTable->GetRoomID(), m_pHostTable->GetTableID(), m_pHostTable->GetPlayerID(0), m_pHostTable->GetPlayerID(1), cbCurrentCard, cbCurrentIndex, curChairID, cbCardIndex[cbCurrentIndex - 1], cbCardIndex[cbCurrentIndex + 1], dwAction, bIsEat_Flag);


				if (bIsEat_Flag == false)
				{
					actions.erase(iter_eat_center);
				}
			}

			vector<BYTE>::iterator iter_eat_right = find(actions.begin(), actions.end(), 3);
			if (iter_eat_right != actions.end() && cbCurrentIndex >= 2 && cbCurrentIndex < 9)
			{
				bool bIsEat_Flag = true;
				if (cbCardIndex[cbCurrentIndex] != 0)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 2] == 1 && cbCardIndex[cbCurrentIndex - 1] == 1 && cbCardIndex[cbCurrentIndex] == 1)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 2] >= 2 || cbCardIndex[cbCurrentIndex - 1] >= 2)
				{
					bIsEat_Flag = false;
				}
				if (cbCardIndex[cbCurrentIndex - 1] != 0 && cbCardIndex[cbCurrentIndex - 2] != 0 && cbCardIndex[cbCurrentIndex - 3] != 0)
				{
					bIsEat_Flag = false;
				}
				// 吃了之后能听牌就要吃
				map<BYTE, vector<tagAnalyseTingNotifyHu>> mpTingNotifyHu;
				DWORD dwAction = AnalyseTingCard17(curChairID, cbHandCardIndex, WeaveItem, cbWeaveCount, mpTingNotifyHu);
				if (dwAction == ACTION_LISTEN && mpTingNotifyHu.size()>0)
				{
					bIsEat_Flag = true;
				}
				if (bIsEat_Flag == false)
				{
					actions.erase(iter_eat_right);
				}
			}
		}
		for (uint32 i = 0; i < actions.size(); i++)
		{
			return actions[i];
		}
		return 0;
	}
	//吃牌判断
	DWORD CMaJiangLogic::EstimateEatCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbCurrentCard)
	{

		//参数效验
		ASSERT(IsValidCard(cbCurrentCard));
		if(IsKingCardData(cbCurrentCard))return ACTION_NULL;

		//变量定义
		BYTE cbExcursion[3]={0,1,2};
		DWORD ItemKind[3]={ACTION_EAT_LEFT,ACTION_EAT_CENTER,ACTION_EAT_RIGHT};

		//吃牌判断
		DWORD EatKind=0;
		BYTE cbFirstIndex=0;
		BYTE cbCurrentIndex=SwitchToCardIndex(cbCurrentCard);
		//中发白
		if(cbCurrentIndex>=31)
		{
			for (BYTE i=0;i<getArrayLen(ItemKind);i++)
			{
				BYTE cbValueIndex=cbCurrentIndex%9;
				if ((cbValueIndex>=cbExcursion[i])&&((cbValueIndex-cbExcursion[i])<=4))
				{
					//吃牌判断
					cbFirstIndex=cbCurrentIndex-cbExcursion[i];
					//过滤
					if(cbFirstIndex<31) continue;
					if ((cbCurrentIndex!=cbFirstIndex)&&(cbCardIndex[cbFirstIndex]==0))
						continue;
					if ((cbCurrentIndex!=(cbFirstIndex+1))&&(cbCardIndex[cbFirstIndex+1]==0))
						continue;
					if ((cbCurrentIndex!=(cbFirstIndex+2))&&(cbCardIndex[cbFirstIndex+2]==0))
						continue;

					//设置类型
					EatKind|=ItemKind[i];
				}
			}

		}
		else if(cbCurrentIndex>=27)
		{
			for (BYTE i=0;i<getArrayLen(ItemKind);i++)
			{
				BYTE cbValueIndex=cbCurrentIndex%9;
				if ((cbValueIndex>=cbExcursion[i])&&((cbValueIndex-cbExcursion[i])<=1))
				{
					//吃牌判断
					cbFirstIndex=cbCurrentIndex-cbExcursion[i];
					if ((cbCurrentIndex!=cbFirstIndex)&&(cbCardIndex[cbFirstIndex]==0))
						continue;
					if ((cbCurrentIndex!=(cbFirstIndex+1))&&(cbCardIndex[cbFirstIndex+1]==0))
						continue;
					if ((cbCurrentIndex!=(cbFirstIndex+2))&&(cbCardIndex[cbFirstIndex+2]==0))
						continue;

					//设置类型
					EatKind|=ItemKind[i];
				}
			}

		}
		else
		{
			for (BYTE i=0;i<getArrayLen(ItemKind);i++)
			{
				BYTE cbValueIndex=cbCurrentIndex%9;
				if ((cbValueIndex>=cbExcursion[i])&&((cbValueIndex-cbExcursion[i])<=6))
				{
					//吃牌判断
					cbFirstIndex=cbCurrentIndex-cbExcursion[i];
					if ((cbCurrentIndex!=cbFirstIndex)&&(cbCardIndex[cbFirstIndex]==0))
						continue;
					if ((cbCurrentIndex!=(cbFirstIndex+1))&&(cbCardIndex[cbFirstIndex+1]==0))
						continue;
					if ((cbCurrentIndex!=(cbFirstIndex+2))&&(cbCardIndex[cbFirstIndex+2]==0))
						continue;

					//设置类型
					EatKind|=ItemKind[i];
				}
			}
		}
		return EatKind;
	}

	//碰牌判断
	DWORD CMaJiangLogic::EstimatePengCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbCurrentCard)
	{

		//参数效验
		ASSERT(IsValidCard(cbCurrentCard));
		if(IsKingCardData(cbCurrentCard))return ACTION_NULL;

		//碰牌判断
		return (cbCardIndex[SwitchToCardIndex(cbCurrentCard)]>=2)?ACTION_PENG:ACTION_NULL;
	}

	//杠牌判断
	DWORD CMaJiangLogic::EstimateGangCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbCurrentCard)
	{

		//参数效验
		ASSERT(IsValidCard(cbCurrentCard));
		if(IsKingCardData(cbCurrentCard))return ACTION_NULL;

		//杠牌判断
		return (cbCardIndex[SwitchToCardIndex(cbCurrentCard)]==3)?ACTION_GANG:ACTION_NULL;
	}

	//杠牌分析
	DWORD CMaJiangLogic::AnalyseGangCard(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, stGangCardResult & GangCardResult)
	{
		//设置变量
		DWORD ActionMask=ACTION_NULL;
		//ZeroMemory(&GangCardResult,sizeof(GangCardResult));
		GangCardResult.Init();

		//手上杠牌 红中也要杠
		for(BYTE i=0;i<MAX_INDEX;i++)
		{
			if(cbCardIndex[i]==4 && !IsKingCardIndex(i))
			{
				BYTE cbCardValue = SwitchToCardData(i);
				if (cbCardValue>0 && emPOKER_BAI>=cbCardValue)
				{
					ActionMask |= ACTION_GANG;
					GangCardResult.cbGangType[GangCardResult.cbCardCount] = 12;
					GangCardResult.cbCardData[GangCardResult.cbCardCount++] = cbCardValue;
				}
			}
		}

		//组合杠牌
		for (BYTE i=0;i<cbWeaveCount;i++)
		{
			if(WeaveItem[i].WeaveKind==ACTION_PENG)
			{
				if(cbCardIndex[SwitchToCardIndex(WeaveItem[i].cbCenterCard)]==1)
				{
					if (WeaveItem[i].cbCenterCard > 0 && emPOKER_BAI >= WeaveItem[i].cbCenterCard)
					{
						ActionMask |= ACTION_GANG;
						GangCardResult.cbGangType[GangCardResult.cbCardCount] = 13;
						GangCardResult.cbCardData[GangCardResult.cbCardCount++] = WeaveItem[i].cbCenterCard;
					}
				}
			}
		}

		return ActionMask;
	}
	//分析扑克
	bool CMaJiangLogic::AnalyseCard(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<stAnalyseItem> & AnalyseItemArray, uint32 & kind)
	{
		kind = 0;
		//取出赖子
		//拷贝数据
		BYTE cbTempCardIndex[MAX_INDEX];
		memcpy(cbTempCardIndex,cbCardIndex,sizeof(cbTempCardIndex));
		BYTE kingCount = 0;
		for(uint8 i=0;i<m_kingCardIndexs.size();++i){
			kingCount += cbTempCardIndex[m_kingCardIndexs[i]];
			cbTempCardIndex[m_kingCardIndexs[i]]=0;//移除赖子
		}
		//没有赖子
		if(kingCount==0){
			return AnalyseCardNoKing(cbCardIndex,WeaveItem,cbWeaveCount,AnalyseItemArray);
		}
		//赖子组合
		static vector<BYTE> combs;
		combs.clear();

		for(uint8 i=0;i<MAX_INDEX;++i)
		{
			if(IsKingCardIndex(i))
				continue;
			for(uint8 j=0;j<kingCount && j<2;++j){
				if(IsNeedKingComb(cbTempCardIndex,i,j)){
					combs.push_back(i);
				}
			}
		}
		static vector< vector<BYTE> > res;
		res.clear();

		Combination(combs,kingCount,res);
		for(uint32 i=0;i<res.size();++i)
		{
			BYTE tempCard[MAX_INDEX];
			memcpy(tempCard,cbTempCardIndex,sizeof(cbTempCardIndex));
			for(uint8 j=0;j<res[i].size();++j){
				tempCard[res[i][j]]++;
			}
			if(AnalyseCardNoKing(tempCard,WeaveItem,cbWeaveCount,AnalyseItemArray)){
				return true;
			}
			//7小队
			//七对
			if(m_pMjCfg->supportSevenPair()){
				kind = IsQiXiaoDui(tempCard,WeaveItem,cbWeaveCount);
				if(kind>0){
					return true;
				}
			}
		}

        return false;
	}
	//分析扑克
	bool CMaJiangLogic::AnalyseCardNoKing(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<stAnalyseItem> & AnalyseItemArray)
	{
		//计算数目
		BYTE cbCardCount=0;
		for(BYTE i=0;i<MAX_INDEX;i++)
			cbCardCount+=cbCardIndex[i];

		//拷贝数据
		BYTE cbTempCardIndex[MAX_INDEX];
		memcpy(cbTempCardIndex,cbCardIndex,sizeof(cbTempCardIndex));

		//效验数目
		//ASSERT((cbCardCount>=2)&&(cbCardCount<=MAX_COUNT)&&((cbCardCount-2)%3==0));
		if ((cbCardCount<2)||(cbCardCount>MAX_COUNT)||((cbCardCount-2)%3!=0))	return false;
		//if ((cbCardCount<2) || (cbCardCount>MAX_COUNT) )	return false;

		//变量定义
		BYTE cbKindItemCount=0;
		stKindItem KindItem[MAX_INDEX-2];
		ZeroMemory(KindItem,sizeof(KindItem));

		//需求判断
		BYTE cbLessKindItem=(cbCardCount-2)/3;
		//ASSERT((cbLessKindItem+cbWeaveCount)==4);

		//单吊判断
		if(cbLessKindItem==0)
		{
			//效验参数
			ASSERT((cbCardCount==2)&&(cbWeaveCount==4));
			//牌眼判断
			for(BYTE i=0;i<MAX_INDEX;i++)
			{
				if(cbCardIndex[i]==2)
				{
					//变量定义
					stAnalyseItem AnalyseItem;
					ZeroMemory(&AnalyseItem,sizeof(AnalyseItem));
					//设置结果
					for (BYTE j=0;j<cbWeaveCount;j++)
					{
						AnalyseItem.WeaveKind[j]=WeaveItem[j].WeaveKind;
						AnalyseItem.cbCenterCard[j]=WeaveItem[j].cbCenterCard;
					}
					AnalyseItem.cbCardEye=SwitchToCardData(i);
					//插入结果
					AnalyseItemArray.push_back(AnalyseItem);
					//LOG_DEBUG("单吊,将牌:%d",AnalyseItem.cbCardEye);
					return true;
				}
			}
			return false;
		}

		//拆分分析
		if(cbCardCount>=3)
		{
			for(BYTE i=0;i<MAX_INDEX;i++)
			{
				//同牌判断
				if(cbCardIndex[i]>=3)
				{
					ASSERT( cbKindItemCount < getArrayLen(KindItem) );
					KindItem[cbKindItemCount].cbCenterCard=SwitchToCardData(i);
					KindItem[cbKindItemCount].cbCardIndex[0]=i;
					KindItem[cbKindItemCount].cbCardIndex[1]=i;
					KindItem[cbKindItemCount].cbCardIndex[2]=i;
					KindItem[cbKindItemCount++].WeaveKind=ACTION_PENG;
				}

				//连牌判断
				if ((i<(MAX_INDEX-2-7))&&(cbCardIndex[i]>0)&&((i%9)<7))
				{
					for (BYTE j=1;j<=cbCardIndex[i];j++)
					{
						if ((cbCardIndex[i+1]>=j)&&(cbCardIndex[i+2]>=j))
						{
							ASSERT( cbKindItemCount < getArrayLen(KindItem) );
							KindItem[cbKindItemCount].cbCenterCard=SwitchToCardData(i);
							KindItem[cbKindItemCount].cbCardIndex[0]=i;
							KindItem[cbKindItemCount].cbCardIndex[1]=i+1;
							KindItem[cbKindItemCount].cbCardIndex[2]=i+2;
							KindItem[cbKindItemCount++].WeaveKind=ACTION_EAT_LEFT;
						}
					}
				}


			}
			// 东南西北中发白(暂时不处理toney)

		}
		//组合分析
		if(cbKindItemCount>=cbLessKindItem)
		{
			//变量定义
			BYTE cbCardIndexTemp[MAX_INDEX];
			ZeroMemory(cbCardIndexTemp,sizeof(cbCardIndexTemp));

			//变量定义
			BYTE cbIndex[MAX_WEAVE]={0,1,2,3};
			stKindItem * pKindItem[MAX_WEAVE];
			ZeroMemory(&pKindItem,sizeof(pKindItem));

			//开始组合
			do
			{
				//设置变量
				memcpy(cbCardIndexTemp,cbCardIndex,sizeof(cbCardIndexTemp));
				for (BYTE i=0;i<cbLessKindItem;i++)
					pKindItem[i]=&KindItem[cbIndex[i]];

				//数量判断
				bool bEnoughCard=true;
				for (BYTE i=0;i<cbLessKindItem*3;i++)
				{
					//存在判断
					BYTE cbCardIndex=pKindItem[i/3]->cbCardIndex[i%3];
					if (cbCardIndexTemp[cbCardIndex]==0)
					{
						bEnoughCard=false;
						break;
					}
					else
						cbCardIndexTemp[cbCardIndex]--;
				}

				//胡牌判断
				if(bEnoughCard==true)
				{
					//牌眼判断
					BYTE cbCardEye=0;
					//是否继续
					bool bContinue=true;
					if(bContinue==true)
					{
						for (BYTE i=0;i<MAX_INDEX;i++)
						{
							if (cbCardIndexTemp[i]==2)
							{
								//眼牌数据
								cbCardEye=SwitchToCardData(i);
								//是否继续
								bContinue=false;
								break;
							}
						}
					}
					//组合类型
					if(cbCardEye!=0)
					{
						//变量定义
						stAnalyseItem AnalyseItem;
						ZeroMemory(&AnalyseItem,sizeof(AnalyseItem));

						//设置组合
						for (BYTE i=0;i<cbWeaveCount;i++)
						{
							AnalyseItem.WeaveKind[i]=WeaveItem[i].WeaveKind;
							AnalyseItem.cbCenterCard[i]=WeaveItem[i].cbCenterCard;
						}
						//设置牌型
						for (BYTE i=0;i<cbLessKindItem;i++)
						{
							AnalyseItem.WeaveKind[i+cbWeaveCount]=pKindItem[i]->WeaveKind;
							AnalyseItem.cbCenterCard[i+cbWeaveCount]=pKindItem[i]->cbCenterCard;
						}
						//设置牌眼
						AnalyseItem.cbCardEye=cbCardEye;
						//插入结果
						AnalyseItemArray.push_back(AnalyseItem);
						//LOG_DEBUG("普通胡牌,将牌:%d",AnalyseItem.cbCardEye);
					}
				}
				//设置索引
				if(cbIndex[cbLessKindItem-1]==(cbKindItemCount-1))
				{
					BYTE i = 0;
					for(i=cbLessKindItem-1;i>0;i--)
					{
						if ((cbIndex[i-1]+1)!=cbIndex[i])
						{
							BYTE cbNewIndex=cbIndex[i-1];
							for (BYTE j=(i-1);j<cbLessKindItem;j++)
								cbIndex[j]=cbNewIndex+j-i+2;
							break;
						}
					}
					if (i==0)
						break;
				}
				else
					cbIndex[cbLessKindItem-1]++;

			} while (true);

		}
		//硬胡分析
		if(AnalyseItemArray.size()>0)
			return true;

		return false;
	}
	//听牌分析
	DWORD CMaJiangLogic::AnalyseTingCard19(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<tagAnalyseTingNotifyHu > & vecNotifyHuCard)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));

		vecNotifyHuCard.clear();

		BYTE cbCardCount = 0;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			cbCardCount += cbCardIndexTemp[i];
		}
		bool bIsHaveEnoughCard = true;
		if (cbWeaveCount == 0)
		{
			if (cbCardCount != 13)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 1)
		{
			if (cbCardCount != 10)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 2)
		{
			if (cbCardCount != 7)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 3)
		{
			if (cbCardCount != 4)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 4)
		{
			if (cbCardCount != 1)
			{
				bIsHaveEnoughCard = false;
			}
		}
		if (bIsHaveEnoughCard == false)
		{
			return ACTION_NULL;
		}


		DWORD dwChiHuRight = 0;
		DWORD action = ACTION_NULL;
		for (BYTE j = 0; j < MAX_INDEX; j++)
		{
			BYTE cbCurrentCard = SwitchToCardData(j);
			//uint32 uChiHuCardPoolCount = GetAnalyseChiHuCardPoolCount(chairID, cbCurrentCard);
			//if (uChiHuCardPoolCount == 0)
			//{
			//	continue;
			//}
			stChiHuResult chr;
			DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);
			//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);

			if (ACTION_CHI_HU == dwAction)
			{
				action = ACTION_LISTEN;
				tagAnalyseTingNotifyHu NotifyHuCard;
				NotifyHuCard.cbCard = cbCurrentCard;
				NotifyHuCard.score = GetAnalyseChiHuScore(chr);
				NotifyHuCard.cbCount = GetAnalyseChiHuCardCount(chairID, cbCurrentCard);
				vecNotifyHuCard.push_back(NotifyHuCard);
			}
		}
		return action;
	}

	//听牌分析(能和)
	DWORD CMaJiangLogic::AnalyseTingCard18(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));

		stChiHuResult chr;
		DWORD dwChiHuRight = 0;
		for (BYTE j = 0; j < MAX_INDEX; j++)
		{
			BYTE cbCurrentCard = SwitchToCardData(j);
			uint32 uCount = GetAnalyseChiHuCardPoolCount(0, cbCurrentCard);
			if (uCount > 0)
			{
				if (ACTION_CHI_HU == AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false))
				{
					return ACTION_LISTEN;
				}
			}

		}
		return ACTION_NULL;
	}


	//0 - 14
	//1 - 11
	//2 - 8
	//3 - 5
	//4 - 2

	//听牌分析
	DWORD CMaJiangLogic::AnalyseTingCard17(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, map<BYTE, vector<tagAnalyseTingNotifyHu>>& mpTingNotifyHu)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
		BYTE cbCardCount = 0;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			cbCardCount += cbCardIndexTemp[i];
		}
		bool bIsHaveEnoughCard = true;
		if (cbWeaveCount == 0)
		{
			if (cbCardCount != 14)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 1)
		{
			if (cbCardCount != 11)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 2)
		{
			if (cbCardCount != 8)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 3)
		{
			if (cbCardCount != 5)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 4)
		{
			if (cbCardCount != 2)
			{
				bIsHaveEnoughCard = false;
			}
		}
		if (bIsHaveEnoughCard == false)
		{
			return ACTION_NULL;
		}

		mpTingNotifyHu.clear();

		DWORD dwChiHuRight = 0;
		DWORD action = ACTION_NULL;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			//LOG_DEBUG("PrintfcbCardIndexTemp - uid:%d,i:%d,cbCardIndexTemp:%d", uid, i, cbCardIndexTemp[i]);

			if (cbCardIndexTemp[i] == 0) continue;
			cbCardIndexTemp[i]--;

			vector<tagAnalyseTingNotifyHu> vecTingNotifyHu;

			BYTE cbOutCard = SwitchToCardData(i);

			for (BYTE j = 0; j < MAX_INDEX; j++)
			{
				BYTE cbCurrentCard = SwitchToCardData(j);
				uint32 uChiHuCardPoolCount = GetAnalyseChiHuCardPoolCount(chairID, cbCurrentCard);
				if (uChiHuCardPoolCount == 0)
				{
					continue;
				}

				stChiHuResult chr;
				DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);

				//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);

				if (ACTION_CHI_HU == dwAction)
				{
					action = ACTION_LISTEN;

					//outCards.push_back(SwitchToCardData(i));
					//huCards.push_back(cbCurrentCard);

					tagAnalyseTingNotifyHu TingNotifyHu;
					TingNotifyHu.cbCard = cbCurrentCard;
					TingNotifyHu.score = GetAnalyseChiHuScore(chr);
					TingNotifyHu.cbCount = uChiHuCardPoolCount;
					TingNotifyHu.chiHuResult = chr;
					vecTingNotifyHu.push_back(TingNotifyHu);
				}
			}
			if (vecTingNotifyHu.size() > 0)
			{
				mpTingNotifyHu.insert(make_pair(cbOutCard, vecTingNotifyHu));
			}
			cbCardIndexTemp[i]++;
		}
		return action;
	}


	//听牌分析
	DWORD CMaJiangLogic::AnalyseTingCard16(uint8 chairID,BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<tagAnalyseTingNotifyHu > & vecNotifyHuCard)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
		
		vecNotifyHuCard.clear();

		BYTE cbCardCount = 0;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			cbCardCount += cbCardIndexTemp[i];
		}
		bool bIsHaveEnoughCard = true;
		if (cbWeaveCount == 0)
		{
			if (cbCardCount != 13)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 1)
		{
			if (cbCardCount != 10)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 2)
		{
			if (cbCardCount != 7)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 3)
		{
			if (cbCardCount != 4)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 4)
		{
			if (cbCardCount != 1)
			{
				bIsHaveEnoughCard = false;
			}
		}
		if (bIsHaveEnoughCard == false)
		{
			return ACTION_NULL;
		}


		DWORD dwChiHuRight = 0;
		DWORD action = ACTION_NULL;
		for (BYTE j = 0; j < MAX_INDEX; j++)
		{
			BYTE cbCurrentCard = SwitchToCardData(j);
			uint32 uChiHuCardPoolCount = GetAnalyseChiHuCardPoolCount(chairID, cbCurrentCard);
			if (uChiHuCardPoolCount == 0)
			{
				continue;
			}
			stChiHuResult chr;
			DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);
			//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);

			if (ACTION_CHI_HU == dwAction)
			{
				action = ACTION_LISTEN;
				tagAnalyseTingNotifyHu NotifyHuCard;
				NotifyHuCard.cbCard = cbCurrentCard;
				NotifyHuCard.score = GetAnalyseChiHuScore(chr);
				NotifyHuCard.cbCount = uChiHuCardPoolCount;
				vecNotifyHuCard.push_back(NotifyHuCard);
			}
		}
		return action;
	}

		//0 - 14
		//1 - 11
		//2 - 8
		//3 - 5
		//4 - 2

	//听牌分析
	DWORD CMaJiangLogic::AnalyseTingCard15(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, map<BYTE, vector<tagAnalyseTingNotifyHu>>& mpTingNotifyHu)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
		BYTE cbCardCount = 0;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			cbCardCount += cbCardIndexTemp[i];
		}
		bool bIsHaveEnoughCard = true;
		if (cbWeaveCount == 0)
		{
			if (cbCardCount != 14)
			{
				bIsHaveEnoughCard = false;
			}			
		}
		else if (cbWeaveCount == 1)
		{
			if (cbCardCount != 11)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 2)
		{
			if (cbCardCount != 8)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 3)
		{
			if (cbCardCount != 5)
			{
				bIsHaveEnoughCard = false;
			}
		}
		else if (cbWeaveCount == 4)
		{
			if (cbCardCount != 2)
			{
				bIsHaveEnoughCard = false;
			}
		}
		if (bIsHaveEnoughCard == false)
		{
			return ACTION_NULL;
		}

		mpTingNotifyHu.clear();
		m_bIsAnalyseTingCard15 = true;
		
		DWORD dwChiHuRight = 0;
		DWORD action = ACTION_NULL;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			//LOG_DEBUG("PrintfcbCardIndexTemp - uid:%d,i:%d,cbCardIndexTemp:%d", uid, i, cbCardIndexTemp[i]);

			if (cbCardIndexTemp[i] == 0) continue;
			cbCardIndexTemp[i]--;

			vector<tagAnalyseTingNotifyHu> vecTingNotifyHu;

			BYTE cbOutCard = SwitchToCardData(i);

			for (BYTE j = 0; j < MAX_INDEX; j++)
			{
				BYTE cbCurrentCard = SwitchToCardData(j);
				stChiHuResult chr;
				DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);

				//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);

				if (ACTION_CHI_HU == dwAction)
				{
					action = ACTION_LISTEN;

					//outCards.push_back(SwitchToCardData(i));
					//huCards.push_back(cbCurrentCard);

					tagAnalyseTingNotifyHu TingNotifyHu;
					TingNotifyHu.cbCard = cbCurrentCard;
					TingNotifyHu.score = GetAnalyseChiHuScore(chr);
					TingNotifyHu.cbCount = GetAnalyseChiHuCardCount(chairID, cbCurrentCard);
					TingNotifyHu.chiHuResult = chr;
					vecTingNotifyHu.push_back(TingNotifyHu);
					//break;
				}
			}
			if (vecTingNotifyHu.size() > 0)
			{
				mpTingNotifyHu.insert(make_pair(cbOutCard, vecTingNotifyHu));
			}
			cbCardIndexTemp[i]++;
		}
		m_bIsAnalyseTingCard15 = false;
		return action;
	}




    //听牌分析
    DWORD CMaJiangLogic::AnalyseTingCard14(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[],BYTE cbWeaveCount,vector<BYTE>& outCards)
    {
        //复制数据
        BYTE cbCardIndexTemp[MAX_INDEX];
        memcpy(cbCardIndexTemp,cbCardIndex,sizeof(cbCardIndexTemp));
        outCards.clear();

        stChiHuResult chr;
        DWORD dwChiHuRight=0;
        DWORD action = ACTION_NULL;
        for(BYTE i = 0; i < MAX_INDEX; i++ )
        {
			//LOG_DEBUG("PrintfcbCardIndexTemp - uid:%d,i:%d,cbCardIndexTemp:%d", uid, i, cbCardIndexTemp[i]);

            if( cbCardIndexTemp[i] == 0 ) continue;
            cbCardIndexTemp[i]--;

            for(BYTE j = 0;j < MAX_INDEX;j++)
            {
                BYTE cbCurrentCard = SwitchToCardData(j);

				DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);

				//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);
				
                if(ACTION_CHI_HU == dwAction)
				{
                    action = ACTION_LISTEN;
                    outCards.push_back(SwitchToCardData(i));
                    break;
                }
            }
            cbCardIndexTemp[i]++;
        }
        return action;
    }

	//听牌分析(能和)
	DWORD CMaJiangLogic::AnalyseTingCard13(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));

		stChiHuResult chr;
		DWORD dwChiHuRight = 0;
		for (BYTE j = 0; j < MAX_INDEX; j++)
		{
			BYTE cbCurrentCard = SwitchToCardData(j);
			if (ACTION_CHI_HU == AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false)) {
				return ACTION_LISTEN;
			}
		}
		return ACTION_NULL;
	}

	//听牌分析
	DWORD CMaJiangLogic::AnalyseTingCard12(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<BYTE>& outCards, vector<BYTE>& huCards)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
		outCards.clear();
		huCards.clear();

		stChiHuResult chr;
		DWORD dwChiHuRight = 0;
		DWORD action = ACTION_NULL;
		for (BYTE i = 0; i < MAX_INDEX; i++)
		{
			//LOG_DEBUG("PrintfcbCardIndexTemp - uid:%d,i:%d,cbCardIndexTemp:%d", uid, i, cbCardIndexTemp[i]);

			if (cbCardIndexTemp[i] == 0) continue;
			cbCardIndexTemp[i]--;

			for (BYTE j = 0; j < MAX_INDEX; j++)
			{
				BYTE cbCurrentCard = SwitchToCardData(j);

				DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);

				//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);

				if (ACTION_CHI_HU == dwAction)
				{
					action = ACTION_LISTEN;
					outCards.push_back(SwitchToCardData(i));
					huCards.push_back(cbCurrentCard);
					break;
				}
			}
			cbCardIndexTemp[i]++;
		}
		return action;
	}
	//听牌分析
	DWORD CMaJiangLogic::AnalyseTingCard11(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<BYTE>& huCards)
	{
		//复制数据
		BYTE cbCardIndexTemp[MAX_INDEX];
		memcpy(cbCardIndexTemp, cbCardIndex, sizeof(cbCardIndexTemp));
		huCards.clear();

		stChiHuResult chr;
		DWORD dwChiHuRight = 0;
		DWORD action = ACTION_NULL;
		for (BYTE j = 0; j < MAX_INDEX; j++)
		{
			BYTE cbCurrentCard = SwitchToCardData(j);

			DWORD dwAction = AnalyseChiHuCard(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, dwChiHuRight, chr, false);

			//LOG_DEBUG("PrintfcbCurrentCard - uid:%d,i:%d,cbCardIndexTemp:%d,cbCurrentCard:0x%02X,cbWeaveCount:%d,dwAction:%d,", uid, i, cbCardIndexTemp[i], cbCurrentCard, cbWeaveCount, dwAction);

			if (ACTION_CHI_HU == dwAction)
			{
				action = ACTION_LISTEN;
				huCards.push_back(cbCurrentCard);
				//break;
			}
		}
		return action;
	}
	//补杠听牌
	DWORD CMaJiangLogic::AnalyseBuGangTing(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[],BYTE cbWeaveCount,BYTE cbCard)
	{
		BYTE cbCardIndexTemp[MAX_INDEX];
		stWeaveItem WeaveItemTmp[MAX_WEAVE];
		BYTE WeaveCountTmp = cbWeaveCount;
		memcpy(cbCardIndexTemp,cbCardIndex,sizeof(cbCardIndexTemp));
		memcpy(WeaveItemTmp,WeaveItem,sizeof(WeaveItemTmp));

		BYTE cardIndex = SwitchToCardIndex(cbCard);

		if(cbCardIndexTemp[cardIndex] == 1){
			cbCardIndexTemp[cardIndex] = 0;
		}else{
			cbCardIndexTemp[cardIndex] = 0;
			WeaveItemTmp[WeaveCountTmp].cbPublicCard = FALSE;
			WeaveItemTmp[WeaveCountTmp].WeaveKind    = ACTION_GANG;
			WeaveItemTmp[WeaveCountTmp].cbCenterCard = cbCard;
			WeaveItemTmp[WeaveCountTmp].wProvideUser = 0;
			WeaveCountTmp++;
		}
		return AnalyseTingCard13(cbCardIndexTemp,WeaveItemTmp,WeaveCountTmp);
	}

	//吃胡分析
	DWORD CMaJiangLogic::AnalyseChiHuCard(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, BYTE cbCurrentCard, DWORD dwChiHuRight, stChiHuResult & ChiHuResult,bool bZiMo )
	{
        //设置变量
        ZeroMemory(&ChiHuResult,sizeof(ChiHuResult));
		ChiHuResult.ChiHuCard = cbCurrentCard;

		//变量定义
		stActionOper ChiHuKind;

        //构造扑克
        BYTE cbCardIndexTemp[MAX_INDEX];
        memcpy(cbCardIndexTemp,cbCardIndex,sizeof(cbCardIndexTemp));

        //插入扑克
		if (cbCurrentCard != 0)
		{
			cbCardIndexTemp[SwitchToCardIndex(cbCurrentCard)]++;
		}
		
        //自模权位
        if(bZiMo)
        {
            dwChiHuRight|= CHR_ZI_MO;
        }



        //权位调整 杠胡+自摸 = 杠上花
        if((dwChiHuRight&CHR_QIANG_GANG)&&(dwChiHuRight&CHR_ZI_MO))
        {
            dwChiHuRight &= ~CHR_QIANG_GANG;
            dwChiHuRight |= CHR_GANG_FLOWER;
        }

		//if (m_pHostTable->GetHuCardPlayerID() > 0)
		//{
		//	LOG_DEBUG("GetHuCardPlayerID:%d,bZiMo:%d,dwChiHuRight:%d,cbCurrentCard:0x%02X", m_pHostTable->GetHuCardPlayerID(), bZiMo, dwChiHuRight, cbCurrentCard);
		//}
        // 处理特殊胡牌类型
		/*if(m_pMjCfg->onlyZimo() && !bZiMo){//是否仅支持自摸
			return ACTION_NULL;
		}*/
		bool bIsQingYiSe = IsQingYiSe(cbCardIndexTemp, WeaveItem, cbWeaveCount, cbCurrentCard, bZiMo);
        //七对
        if(m_pMjCfg->supportSevenPair()){
			uint32 kind = IsQiXiaoDui(cbCardIndexTemp,WeaveItem,cbWeaveCount);
			if(kind>0)
			{
				ChiHuKind.add(kind);

				if (bIsQingYiSe)
				{
					ChiHuKind.add(HUTYPE_QING_YI_SE);
				}
			}
        }
        //13离
        if(m_pMjCfg->supportThirteen() && cbWeaveCount==0 &&IsNeatAlone(cbCardIndexTemp)==true){
			ChiHuKind.add(HUTYPE_THIRTEEN);
        }
		//将将胡
		if(m_pMjCfg->supportJiangJiangHu() && IsJiangJiangHu(cbCardIndexTemp,WeaveItem,cbWeaveCount)){
			ChiHuKind.add(HUTYPE_JIANG_JIANG);
		}
		//红中宝4红中直接胡
		if(m_pHostTable->GetSendCardCount()==1 && bZiMo && m_pMjCfg->hasHongZhongCard() && m_pMjCfg->supportSuperCard()){
			if(IsHongZhongHu(cbCardIndexTemp, WeaveItem,cbWeaveCount,cbCurrentCard)){
				ChiHuKind.add(HUTYPE_HONGZHONG);
			}
		}

        static vector<stAnalyseItem> AnalyseItemArray;
        AnalyseItemArray.clear();

        //分析扑克
		uint32 kind = 0;
        AnalyseCard(cbCardIndexTemp,WeaveItem,cbWeaveCount,AnalyseItemArray,kind);
		if(kind > 0){ ChiHuKind.add(kind); }

        //胡牌分析
        if(AnalyseItemArray.size()>0)
        {
			if(bIsQingYiSe)
			{
				ChiHuKind.add(HUTYPE_QING_YI_SE);
			}
			if(m_bIsAnalyseTingCard15 == false && IsQuanQiuRen(cbCardIndexTemp,WeaveItem,cbWeaveCount,cbCurrentCard, bZiMo))
			{
				ChiHuKind.add(HUTYPE_QUAN_QIU_REN);
			}
			if(IsMenQing(cbCardIndexTemp,WeaveItem,cbWeaveCount,cbCurrentCard, bZiMo)){
				ChiHuKind.add(HUTYPE_MEN_QING);
			}
			else
			{
				// 如果没有门前清不算立直
				if (dwChiHuRight&CHR_LI_ZHI)
				{
					dwChiHuRight &= ~CHR_LI_ZHI;
				}
			}
            //牌型分析
            for(uint32 i=0;i<AnalyseItemArray.size();i++)
            {
                //变量定义
                bool bLianCard=false,bPengCard=false;
                stAnalyseItem * pAnalyseItem=&AnalyseItemArray[i];
				//LOG_DEBUG("胡牌牌型:%s",PrintHuType(AnalyseItemArray[i]).c_str());

                //牌型分析
                for (BYTE j=0;j<getArrayLen(pAnalyseItem->WeaveKind);j++)
                {
                    DWORD WeaveKind=pAnalyseItem->WeaveKind[j];
                    bPengCard=((WeaveKind&(ACTION_GANG|ACTION_PENG))!=0)?true:bPengCard;
                    bLianCard=((WeaveKind&(ACTION_EAT_LEFT|ACTION_EAT_CENTER|ACTION_EAT_RIGHT))!=0)?true:bLianCard;
                }
                //牌型判断
                //ASSERT((bLianCard==true)||(bPengCard==true));
                //平胡 胡牌时，牌型由4组顺子和万字牌做将组成
    //            if((bLianCard==true)&&(bPengCard==true)){
				//	ChiHuKind.add(HUTYPE_PING_HU);
				//}
    //            if((bLianCard==true)&&(bPengCard==false)){
				//	ChiHuKind.add(HUTYPE_PING_HU);
				//}
                if((bLianCard==false)&&(bPengCard==true)){
					ChiHuKind.add(HUTYPE_PENG_PENG);
				}

				//检查258将限制
				if(m_pMjCfg->isNeedLeader258())
				{
					if(IsNeed258Leader(ChiHuKind))
					{
						if(!IsLeaderCard258(pAnalyseItem->cbCardEye)){
							LOG_DEBUG("不是258将不能胡");
							ChiHuKind.reset();
						}
					}
				}
				if(m_pMjCfg->playType() == MAJIANG_TYPE_GUOBIAO )
				{
					CalcHuCardType(cbCardIndex,WeaveItem,cbWeaveCount,cbCurrentCard, bZiMo,pAnalyseItem,ChiHuKind);
				}
				else if (m_pMjCfg->playType() == MAJIANG_TYPE_TWO_PEOPLE)
				{
					CalcHuCardType(cbCardIndex, WeaveItem, cbWeaveCount, cbCurrentCard, bZiMo, pAnalyseItem, ChiHuKind);
				}
            }
        }

		//LOG_DEBUG("uid:%d,size:%d,kind:%d,isNull:%d,cbWeaveCount:%d,", uid, AnalyseItemArray.size(), kind, ChiHuKind.isNull(), cbWeaveCount);

        //结果判断
        if(!ChiHuKind.isNull())
        {
			//ChiHuKind.add(HUTYPE_PING_HU);
			CheckAddHuCardType(dwChiHuRight, ChiHuKind);

            ChiHuResult.HuKind = ChiHuKind;
            ChiHuResult.dwChiHuRight=dwChiHuRight;
            return ACTION_CHI_HU;
        }

        return ACTION_NULL;
	}

	bool CMaJiangLogic::CheckAddHuCardType(DWORD dwChiHuRight, stActionOper & ChiHuKind)
	{
		//if (m_pHostTable->GetHuCardPlayerID() > 0)
		//{
		//	LOG_DEBUG("GetHuCardPlayerID:%d,dwChiHuRight:%d", m_pHostTable->GetHuCardPlayerID(), dwChiHuRight);
		//}
		//获取牌型
		//if (dwChiHuRight&CHR_GANG_FLOWER)
		//{
		//	ChiHuKind.add(HUTYPE_GANG_FLOWER);
		//}
		if (dwChiHuRight&CHR_DI)
		{
			ChiHuKind.add(HUTYPE_DI_HU);
		}
		if (dwChiHuRight&CHR_TIAN)
		{
			ChiHuKind.add(HUTYPE_TIAN_HU);
		}
		if (dwChiHuRight&CHR_QIANG_GANG)
		{
			ChiHuKind.add(HUTYPE_QIANG_GANG_HU);
			//if (m_pHostTable->GetHuCardPlayerID() > 0)
			//{
			//	LOG_DEBUG("抢杠胡 - GetHuCardPlayerID:%d,dwChiHuRight:%d", m_pHostTable->GetHuCardPlayerID(), dwChiHuRight);
			//}
		}
		if (dwChiHuRight&CHR_RENHU)
		{
			ChiHuKind.add(HUTYPE_REN_HU);
		}
		if (dwChiHuRight&CHR_ZI_MO)
		{
			ChiHuKind.add(HUTYPE_ZI_MO);
		}
		//if (dwChiHuRight&CHR_GANG_FLOWER)
		//{
		//	ChiHuKind.add(HUTYPE_GANG_FLOWER);
		//}
		if (dwChiHuRight&CHR_TIAN_TING)
		{
			ChiHuKind.add(HUTYPE_TIAN_TING);
		}
		if (dwChiHuRight&CHR_LI_ZHI)
		{
			ChiHuKind.add(HUTYPE_LI_ZHI);
		}
		return true;
	}

	uint32 CMaJiangLogic::GetAnalyseChiHuScore(stChiHuResult & result)
	{
		RemoveGuoBiaoFan(result);
		int32 fan = CalcGuoBiaoFan(result);

		return fan;
	}

	uint32 CMaJiangLogic::GetAnalyseChiHuCardCount(uint8 chairID, BYTE cbCard)
	{
		uint32 uCount = 0;

		if (m_pHostTable == NULL)
		{
			return uCount;
		}
		
		list<BYTE>::iterator it = m_pHostTable->m_poolCards.begin();
		for (; it != m_pHostTable->m_poolCards.end(); it++)
		{
			BYTE cbTempCard = *it;
			if (cbTempCard == cbCard)
			{
				uCount++;
			}
		}

		if (chairID >= m_pHostTable->GetPlayerCount())
		{
			return uCount;
		}

		for (uint8 i = 0; i < m_pHostTable->GetPlayerCount(); i++)
		{
			if (chairID == i)
			{
				continue;
			}

			BYTE cardData[MAX_COUNT] = { 0 };
			BYTE cardNum = SwitchToCardData(m_pHostTable->m_cbCardIndex[i], cardData, MAX_COUNT);
			for (uint32 j = 0; j < cardNum; ++j)
			{
				if (cardData[j] == cbCard)
				{
					uCount++;
				}
			}
		}

		return uCount;
	}

	uint32 CMaJiangLogic::GetAnalyseChiHuCardPoolCount(uint8 chairID, BYTE cbCard)
	{
		uint32 uCount = 0;

		if (m_pHostTable == NULL)
		{
			return uCount;
		}

		list<BYTE>::iterator it = m_pHostTable->m_poolCards.begin();
		for (; it != m_pHostTable->m_poolCards.end(); it++)
		{
			BYTE cbTempCard = *it;
			if (cbTempCard == cbCard)
			{
				uCount++;
			}
		}
		return uCount;
	}

	//扑克转换
	BYTE CMaJiangLogic::SwitchToCardData(BYTE cbCardIndex)
	{
		ASSERT(cbCardIndex<MAX_INDEX);
		return ((cbCardIndex/9)<<4)|(cbCardIndex%9+1);
	}

	//扑克转换
	BYTE CMaJiangLogic::SwitchToCardIndex(BYTE cbCardData)
	{
		ASSERT(IsValidCard(cbCardData));
		////计算位置
		return ((cbCardData&MASK_COLOR)>>4)*9+(cbCardData&MASK_VALUE)-1;
	}

	//扑克转换
	BYTE CMaJiangLogic::SwitchToCardData(BYTE cbCardIndex[MAX_INDEX], BYTE cbCardData[MAX_COUNT],BYTE bMaxCount)
	{
		//转换扑克
		BYTE bPosition=0;
		for(BYTE i=0;i<MAX_INDEX;i++)
		{
			if(cbCardIndex[i]!=0)
			{
				for (BYTE j=0;j<cbCardIndex[i];j++)
				{
					//LOG_DEBUG("roomid:%d,tableid:%d,bPosition:%d,bMaxCount:%d,i:%d,j:%d,cbCardIndex[i]:%d",
					//	m_pHostTable->GetRoomID(), m_pHostTable->GetTableID(), bPosition, bMaxCount, i, j, cbCardIndex[i]);
					ASSERT(bPosition<bMaxCount);
					cbCardData[bPosition++]=SwitchToCardData(i);
				}
			}
		}
		return bPosition;
	}
	//扑克转换
	BYTE CMaJiangLogic::SwitchToCardIndex(BYTE cbCardData[], BYTE cbCardCount, BYTE cbCardIndex[MAX_INDEX])
	{
		//设置变量
        ZeroMemory(cbCardIndex,sizeof(BYTE)*MAX_INDEX);

        //转换扑克
        for (BYTE i=0;i<cbCardCount;i++)
        {
            cbCardIndex[SwitchToCardIndex(cbCardData[i])]++;
        }

        return cbCardCount;
	}
    //扑克转换
    BYTE CMaJiangLogic::SwitchToCardIndex(vector<BYTE> vecCardData,BYTE cbCardIndex[MAX_INDEX])
    {
        //设置变量
        ZeroMemory(cbCardIndex,sizeof(BYTE)*MAX_INDEX);

        //转换扑克
        for(BYTE i=0;i<vecCardData.size();i++)
        {
            cbCardIndex[SwitchToCardIndex(vecCardData[i])]++;
        }

        return vecCardData.size();
    }
    // 是不是2、5、8将牌
    bool CMaJiangLogic::IsLeaderCard258(BYTE cardValue){
		if(IsKingCardData(cardValue))
			return true;
        return (GetCardValue(cardValue)%3)==2;
    }
	// 是否需要258将
	bool CMaJiangLogic::IsNeed258Leader(stActionOper& oper)
	{
		if(oper.isExist(HUTYPE_PENG_PENG) || oper.isExist(HUTYPE_QING_YI_SE)
	    || oper.isExist(HUTYPE_QUAN_QIU_REN) || oper.isExist(HUTYPE_JIANG_JIANG)
	    || oper.isExist(HUTYPE_QI_DUI) || oper.isExist(HUTYPE_SUPER_QI_DUI)
	    || oper.isExist(HUTYPE_SUPER2_QI_DUI) || oper.isExist(HUTYPE_THIRTEEN)){
			return false;
		}
		return true;
	}
	// 是否需要替换赖子
	bool CMaJiangLogic::IsNeedKingComb(BYTE cbCardIndex[MAX_INDEX],BYTE cbIndex,BYTE pos){
		if(cbIndex>=27)// 字牌目前不需要替换
			return false;
		if((cbCardIndex[cbIndex]+pos)>4)
			return false;

		return true;
	}

	//13乱
	bool CMaJiangLogic::IsNeatAlone(BYTE cbCardIndex[MAX_INDEX])
	{
		BYTE cbTempCardIndex[MAX_INDEX];
		memcpy(cbTempCardIndex,cbCardIndex,sizeof(BYTE)*MAX_INDEX);

		//计算数目
		BYTE cbCardCount=0;
		for (BYTE i=0;i<MAX_INDEX;i++)
			cbCardCount+=cbTempCardIndex[i];

		//满牌才能全不靠
		if(cbCardCount<14) return false;


		//变量处理
		const BYTE COMMON_TYPE_SUM = 9;
		for (BYTE i=0;i<34;i++)
		{
			//重复字牌
			if(cbTempCardIndex[i] > 1)
			{
				return false;
			}
		}

		BYTE* pKingIndex = 0;
		for(BYTE i=0; i<3; i++)
		{

			pKingIndex = &cbTempCardIndex[i*COMMON_TYPE_SUM];
			for(BYTE j=0; j<COMMON_TYPE_SUM; j++)
			{
				if(pKingIndex[j] > 0)
				{
					for(BYTE k=0; k<2; k++)
					{
						j ++;
						if(j<COMMON_TYPE_SUM)
						{
							if(pKingIndex[j] > 0)
							{
								return false;
							}
						}
					}

				}
			}
		}
		//LOG_DEBUG("十三幺");
		return true;
	}
	//七小对牌
	uint32 CMaJiangLogic::IsQiXiaoDui(const BYTE cbCardIndex[MAX_INDEX], const stWeaveItem WeaveItem[], const BYTE cbWeaveCount)
	{
		//组合判断
		if(cbWeaveCount != 0)
			return HUTYPE_NULL;

		//单牌数目
		BYTE cbReplaceCount = 0;
		BYTE cbFourCount = 0;

		//计算单牌
		for(BYTE i=0;i<MAX_INDEX;i++)
		{
			BYTE cbCardCount=cbCardIndex[i];
			//单牌统计
			if( cbCardCount == 1 || cbCardCount == 3 ){
				cbReplaceCount++;
			}
			//四张统计
			if(cbCardCount == 4){
				cbFourCount++;
			}
		}

		if(cbReplaceCount > 0)return HUTYPE_NULL;
		//七连对
		if(m_pMjCfg->supportSevenPair7())
		{
			//if (cbFourCount==0)
			{
				for (BYTE i = 0; i < MAX_INDEX - 7; ++i)
				{
					bool falg = true;
					BYTE j = 0;
					for (; j < 7; ++j)
					{
						if (cbCardIndex[i + j] == 0)
						{
							falg = false;
							break;
						}
					}
					if (falg)
					{
						//LOG_DEBUG("七连对");
						return HUTYPE_LIAN_QI_DUI;
					}
				}
			}
		}
		if (m_pMjCfg->supportSevenPair3())
		{
			if (cbFourCount >= 3 && m_pMjCfg->supportSevenPair3()) {
				//LOG_DEBUG("三豪华七小对");
				return HUTYPE_SUPER3_QI_DUI;
			}
			if (cbFourCount >= 2) {
				//LOG_DEBUG("双豪华七小对");
				return HUTYPE_SUPER2_QI_DUI;
			}
			if (cbFourCount == 1) {
				//LOG_DEBUG("豪华七小对");
				return HUTYPE_SUPER_QI_DUI;
			}
		}


		LOG_DEBUG("七对");
		return HUTYPE_QI_DUI;
	}
    // 清一色
    bool CMaJiangLogic::IsQingYiSe(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo)
    {
        BYTE curColor = 0xFF;
		//bool bIsFirst = true;
        for(BYTE i=0;i<MAX_INDEX;++i)
		{
			if(cbCardIndex[i]==0)
				continue;
            BYTE color = GetCardColorValue(SwitchToCardData(i));
            if(curColor == 0xFF)
			{
                curColor = color;
            }
			if (color == emPOKER_COLOR_SUO || color == emPOKER_COLOR_TONG || color == emPOKER_COLOR_ZI)
			{
				return false;
			}
            if(curColor != color)
                return false;
        }
        for(BYTE i=0;i<cbWeaveCount;++i)
		{
            BYTE color = GetCardColorValue(WeaveItem[i].cbCenterCard);
            if(curColor == 0xFF)
			{
                curColor = color;
				//bIsFirst = false;
            }
			if (color == emPOKER_COLOR_SUO || color == emPOKER_COLOR_TONG || color == emPOKER_COLOR_ZI)
			{
				return false;
			}
            if(curColor != color)
                return false;
        }
		//LOG_DEBUG("清一色");
        return true;
    }
    // 全求人(吃碰杠四副后，手中只剩一张牌，必须吃胡胡牌，乱将)
    bool CMaJiangLogic::IsQuanQiuRen(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo)
    {
        if(cbWeaveCount == 4 && !bZimo) {
			//LOG_DEBUG("全求人");
			return true;
		}

        return false;
    }

    // 门清(没有吃碰明杠)
    bool CMaJiangLogic::IsMenQing(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo)
    {
		//if(!bZimo || m_pHostTable->GetSendCardCount() == 1)return false;
		if (bZimo)
		{
			return false;
		}
		for(uint8 i=0;i<cbWeaveCount;++i){
			if(WeaveItem[i].WeaveKind != ACTION_GANG || WeaveItem[i].cbPublicCard == 1)
				return false;
		}

        return true;
    }
    void CMaJiangLogic::CheckMinHu(BYTE cbCardIndex[MAX_INDEX],stActionOper& oper)
    {
        bool bFlag = true;
        // 板板胡
		if(m_pMjCfg->IsOpenHuType(MINHU_BANBAN_HU)){
			for (BYTE i = 0; i < MAX_INDEX; ++i) {
				if (cbCardIndex[i] > 0) {
					BYTE value = GetCardValue(SwitchToCardData(i));
					if (value % 3 == 2) {
						bFlag = false;
						break;
					}
				}
			}
			if (bFlag == true) {
				oper.add(MINHU_BANBAN_HU);
				LOG_DEBUG("板板胡");
			}
		}
        // 六六顺,小四喜
		BYTE num = 0;
		for(BYTE i=0;i<MAX_INDEX;++i)
		{
			if(cbCardIndex[i] >= 3){
				num++;
				if(m_pMjCfg->IsOpenHuType(MINHU_SI_XI)) {
					if(cbCardIndex[i] == 4) {
						oper.add(MINHU_SI_XI);
						LOG_DEBUG("小四喜");
					}
				}
			}
		}
		if(m_pMjCfg->IsOpenHuType(MINHU_LIULIU_SHUN)){
			if(num >= 2){
				oper.add(MINHU_LIULIU_SHUN);
				LOG_DEBUG("六六顺");
			}
		}

        // 缺一色
		if(m_pMjCfg->IsOpenHuType(MINHU_QUE_YI_SE)){
			BYTE numColor[4];
			memset(numColor, 0, sizeof(numColor));
			for (BYTE i = 0; i < MAX_INDEX; ++i) {
				if (cbCardIndex[i] > 0) {
					numColor[GetCardColorValue(SwitchToCardData(i))] = 1;
				}
			}
			if ((numColor[0] + numColor[1] + numColor[2]) < 3) {
				oper.add(MINHU_QUE_YI_SE);
				LOG_DEBUG("缺一色");
			}
		}
		// 节节高
		if(m_pMjCfg->IsOpenHuType(MINHU_JIE_JIE_GAO)){
			for (BYTE i = 0; i < MAX_INDEX - 3; ++i) {
				if (cbCardIndex[i] > 1 && cbCardIndex[i + 1] > 1 && cbCardIndex[i + 2] > 1) {
					oper.add(MINHU_JIE_JIE_GAO);
					LOG_DEBUG("节节高");
					break;
				}
			}
		}
		// 3同
		if(m_pMjCfg->IsOpenHuType(MINHU_SAN_TONG)){
			for(uint8 i=0;i<9;++i)
			{
				if(cbCardIndex[SwitchToCardIndex(emPOKER_WAN1+i)]>1
				&& cbCardIndex[SwitchToCardIndex(emPOKER_SUO1+i)]>1
				&& cbCardIndex[SwitchToCardIndex(emPOKER_TONG1+i)]>1){
					oper.add(MINHU_SAN_TONG);
					LOG_DEBUG("三同");
					break;
				}
			}
		}
		// 一枝花
		if(m_pMjCfg->IsOpenHuType(MINHU_YIZHIHUA))
		{
			//玩家手上只有一只将牌，且这只将牌数字为“5”
			if(m_pMjCfg->playType() == MAJIANG_TYPE_CHANGSHA) {
				if (cbCardIndex[SwitchToCardIndex(emPOKER_WAN1 + 1)] == 0
					&& cbCardIndex[SwitchToCardIndex(emPOKER_WAN1 + 7)] == 0
					&& cbCardIndex[SwitchToCardIndex(emPOKER_SUO1 + 1)] == 0
					&& cbCardIndex[SwitchToCardIndex(emPOKER_SUO1 + 7)] == 0
					&& cbCardIndex[SwitchToCardIndex(emPOKER_TONG1 + 1)] == 0
					&& cbCardIndex[SwitchToCardIndex(emPOKER_TONG1 + 7)] == 0) {
					BYTE num = 0;
					num += cbCardIndex[SwitchToCardIndex(emPOKER_WAN1 + 4)];
					num += cbCardIndex[SwitchToCardIndex(emPOKER_SUO1 + 4)];
					num += cbCardIndex[SwitchToCardIndex(emPOKER_TONG1 + 4)];
					if (num == 1) {
						LOG_DEBUG("一枝花1");
						oper.add(MINHU_YIZHIHUA);
					}
				}
			}
			//玩家手牌中某花色只有一张，且这张牌为“5”
			BYTE colorNum[3];
			memset(colorNum,0,sizeof(colorNum));
			for(uint8 i=0;i<27;++i)
			{
				BYTE color = GetCardColorValue(SwitchToCardData(i));
				colorNum[color] += cbCardIndex[i];
			}
			for(uint8 i=0;i<3;++i)
			{
				//LOG_DEBUG("一直花2:--%d---%d--%d",colorNum[i],SwitchToCardIndex(MakeCardData(4,i)),cbCardIndex[SwitchToCardIndex(MakeCardData(4,i))]);
				if(colorNum[i] == 1 && cbCardIndex[SwitchToCardIndex(MakeCardData(4,i))] == 1)
				{
					LOG_DEBUG("一枝花2");
					oper.add(MINHU_YIZHIHUA);
					break;
				}
			}
		}
		// 金童玉女
		if(m_pMjCfg->IsOpenHuType(MINHU_JINTONG_YUNV)){
			//玩家手里有一对二筒跟一对二条
			if(cbCardIndex[SwitchToCardIndex(emPOKER_TONG1+1)] >= 2 && cbCardIndex[SwitchToCardIndex(emPOKER_SUO1+1)] >= 2){
				LOG_DEBUG("金童玉女");
				oper.add(MINHU_JINTONG_YUNV);
			}
		}
		// 一点红
		if(m_pMjCfg->IsOpenHuType(MINHU_YIDIANHONG)){
			BYTE num = 0;
			for(uint8 i=0;i<MAX_INDEX;++i){
				if(cbCardIndex[i] > 0 && IsLeaderCard258(SwitchToCardData(i))){
					num += cbCardIndex[i];
				}
			}
			if(num == 1){
				LOG_DEBUG("一点红");
				oper.add(MINHU_YIDIANHONG);
			}
		}

    }
	// 国标麻将算番
	uint32 CMaJiangLogic::CalcGuoBiaoFan(stChiHuResult& result)
	{
		uint32 fan=0;

		//88番
		for(uint8 i=0;i<HUTYPE_MAX_TYPE;++i)
		{
			if(!result.IsKind(i))continue;
			switch(i)
			{
			//88
			case HUTYPE_DA_SI_XI:
			case HUTYPE_DA_SAN_YUAN:
			case HUTYPE_SI_GANG:
			case HUTYPE_LIAN_QI_DUI:
			case HUTYPE_BAI_WAN_DAN:
			case HUTYPE_TIAN_HU:
			case HUTYPE_DI_HU:
			case HUTYPE_REN_HU: // --
				{
					fan += 88;//88番
					result.HuKind.setScore(i, 88);
				}break;
			//64
			case HUTYPE_XIAO_SI_XI:
			case HUTYPE_XIAO_SAN_YUAN:
			case HUTYPE_ZI_YI_SE:
			case HUTYPE_SI_AN_KE:
			case HUTYPE_YISE_SHUANG_LONG:
				{
					fan += 64;//64番
					result.HuKind.setScore(i, 64);
				}break;
			//48
			case HUTYPE_YISE_SITONGSHUN:
			case HUTYPE_YISE_SIJIEGAO:
				{
					fan+=48;
					result.HuKind.setScore(i, 48);
				}break;
			//32
			case HUTYPE_YISE_SIBUGAO:
			case HUTYPE_SAN_GANG:
			case HUTYPE_HUN_YAOJIU:
				{
					fan+=32;
					result.HuKind.setScore(i, 32);
				}break;
			//24
			case HUTYPE_QI_DUI:
			case HUTYPE_QING_YI_SE:
			case HUTYPE_YISE_SANTONGSHUN:
			case HUTYPE_YISE_SANJIEGAO:
				{
					fan+=24;
					result.HuKind.setScore(i, 24);
				}break;
			//16
			case HUTYPE_QING_LONG:
			case HUTYPE_YISE_SANBUGAO:
			case HUTYPE_SAN_ANKE:
			case HUTYPE_TIAN_TING:
				{
					fan+=16;
					result.HuKind.setScore(i, 16);
				}break;
			//12
			case HUTYPE_DAYU_WU:
			case HUTYPE_XIAOYU_WU:
			case HUTYPE_SAN_FENGKE:
				{
					fan+=12;
					result.HuKind.setScore(i, 12);
				}break;
			//8番
			case HUTYPE_MIAOSHOUHUICHUN:
			case HUTYPE_HAI_DI_LAO_YUE:
			case HUTYPE_GANG_FLOWER:
			case HUTYPE_QIANG_GANG_HU:
				{
					fan+=8;
					result.HuKind.setScore(i, 8);
				}break;
			//6番
			case HUTYPE_PENG_PENG:
			case HUTYPE_HUN_YISE:
			case HUTYPE_QUAN_QIU_REN:
			case HUTYPE_SHUANG_ANGANG:
			case HUTYPE_SHUANG_JIANKE:
				{
					fan+=6;
					result.HuKind.setScore(i, 6);
				}break;
			//4番
			case HUTYPE_QUANDAIYAO:
			case HUTYPE_BUQIUREN:
			case HUTYPE_SHUANG_MINGGANG:
			case HUTYPE_HU_JUEZHANG:
			case HUTYPE_LI_ZHI:
				{
					fan+=4;
					result.HuKind.setScore(i, 4);
				}break;
			//2番
			case HUTYPE_JIAN_KE:
			case HUTYPE_QUAN_FENGKE:
			case HUTYPE_MEN_FENGKE:
			case HUTYPE_MEN_QING:
			case HUTYPE_PING_HU:
			case HUTYPE_SI_GUI_YI:
			case HUTYPE_SHUANG_ANKE:
			case HUTYPE_AN_GANG:
			case HUTYPE_DUAN_YAOJIU:
				{
					fan+=2;
					result.HuKind.setScore(i, 2);
				}break;
			//1番
			case HUTYPE_ERWUBA_JIANG:
			case HUTYPE_YAOJIU_TOU:
			case HUTYPE_BAO_TING:
			case HUTYPE_YIBAN_GAO:
			case HUTYPE_LIAN_LIU:
			case HUTYPE_LAOSHAO_FU:
			case HUTYPE_YAOJIU_KE:
			case HUTYPE_MING_GANG:
			case HUTYPE_BIAN_ZHANG:
			case HUTYPE_KAN_ZHANG:
			case HUTYPE_DANDIAO_JIANG:
			case HUTYPE_ZI_MO:
				{
					fan+=1;
					result.HuKind.setScore(i, 1);
				}break;
			default:
				LOG_ERROR("没有处理的牌型:%d",i);
				break;
			}
		}
		//天地胡88番
		/*
		if(result.IsRight(CHR_TIAN))
		{
			fan+=88;
			result.HuKind.add(HUTYPE_TIAN_HU);
			result.HuKind.setScore(HUTYPE_TIAN_HU, 88);

		}
		if (result.IsRight(CHR_DI))
		{
			fan += 88;
			result.HuKind.add(HUTYPE_DI_HU);
			result.HuKind.setScore(HUTYPE_DI_HU, 88);
		}
		//海底捞月,杠上开花,抢杠胡
		if(result.IsRight(CHR_HAIDILAOYUE)){
			fan+8;
			result.HuKind.add(HUTYPE_HAI_DI_LAO_YUE);
			result.HuKind.setScore(HUTYPE_HAI_DI_LAO_YUE, 8);
		}
		if (result.IsRight(CHR_GANG_FLOWER)) {
			fan + 8;
			result.HuKind.add(HUTYPE_GANG_FLOWER);
			result.HuKind.setScore(HUTYPE_GANG_FLOWER, 8);
		}
		if (result.IsRight(CHR_QIANG_GANG)) {
			fan + 8;
			result.HuKind.add(HUTYPE_QIANG_GANG_HU);
			result.HuKind.setScore(HUTYPE_QIANG_GANG_HU, 8);
		}
		//自摸
		if(result.IsRight(CHR_ZI_MO)){
			fan+=1;
			result.HuKind.add(HUTYPE_ZI_MO);
			result.HuKind.setScore(HUTYPE_ZI_MO, 1);
		}
		*/
		return fan;
	}
	// 国标麻将移除叠加番型
	void CMaJiangLogic::RemoveGuoBiaoFan(stChiHuResult& result)
	{
		for(uint32 i=1;i<HUTYPE_MAX_TYPE;++i)
		{
			if(!result.HuKind.isExist(i))
				continue;
			switch(i)
			{
			case HUTYPE_QUAN_QIU_REN:	// 全求人(不计单调将)
				{
					result.HuKind.del(HUTYPE_DANDIAO_JIANG);
				}break;
			case HUTYPE_DA_SI_XI:	// 大四喜(不计圈风刻,门风刻,碰碰胡,幺九刻)
				{
					result.HuKind.del(HUTYPE_QUAN_FENGKE);
					result.HuKind.del(HUTYPE_MEN_FENGKE);
					result.HuKind.del(HUTYPE_PENG_PENG);
					result.HuKind.del(HUTYPE_YAOJIU_KE);
					result.HuKind.del(HUTYPE_SAN_FENGKE);
				}break;
			case HUTYPE_DA_SAN_YUAN:	// 大三元(不计双箭刻,箭刻)
				{
					result.HuKind.del(HUTYPE_JIAN_KE);
					result.HuKind.del(HUTYPE_SHUANG_JIANKE);
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_SI_GANG:
				{
					result.HuKind.del(HUTYPE_SAN_GANG);
				}break;
			case HUTYPE_LIAN_QI_DUI:	// 连七对(不计七对,清一色,门清,单调)
				{
					result.HuKind.del(HUTYPE_QI_DUI);
					result.HuKind.del(HUTYPE_QING_YI_SE);
					result.HuKind.del(HUTYPE_MEN_QING);
					result.HuKind.del(HUTYPE_DANDIAO_JIANG);
				}break;
			case HUTYPE_BAI_WAN_DAN:	// 百万石(不计清一色)
				{
					result.HuKind.del(HUTYPE_QING_YI_SE);
				}break;
			case HUTYPE_XIAO_SI_XI:	// 小四喜(不计三风刻)
				{
					result.HuKind.del(HUTYPE_SAN_FENGKE);
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_XIAO_SAN_YUAN:	// 小三元(不计双箭刻,箭刻,幺九刻)
				{
					result.HuKind.del(HUTYPE_SHUANG_JIANKE);
					result.HuKind.del(HUTYPE_JIAN_KE);
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_ZI_YI_SE:	// 字一色(不计碰碰胡,混幺九,全带幺,幺九刻)
				{
					result.HuKind.del(HUTYPE_PENG_PENG);
					result.HuKind.del(HUTYPE_HUN_YAOJIU);
					result.HuKind.del(HUTYPE_QUANDAIYAO);
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_SI_AN_KE:	// 四暗刻(不计门清,碰碰胡,三暗刻,双安刻,不求人)
				{
					result.HuKind.del(HUTYPE_MEN_QING);
					result.HuKind.del(HUTYPE_PENG_PENG);
					result.HuKind.del(HUTYPE_SAN_ANKE);
					result.HuKind.del(HUTYPE_SHUANG_ANKE);
					result.HuKind.del(HUTYPE_BUQIUREN);
				}break;
			case HUTYPE_YISE_SHUANG_LONG:	// 一色双龙会(不计平胡,七对,清一色,一般高,老少副)
				{
					result.HuKind.del(HUTYPE_PING_HU);
					result.HuKind.del(HUTYPE_QI_DUI);
					result.HuKind.del(HUTYPE_QING_YI_SE);
					result.HuKind.del(HUTYPE_YIBAN_GAO);
					result.HuKind.del(HUTYPE_LAOSHAO_FU);
				}break;
			case HUTYPE_YISE_SITONGSHUN:	// 一色四同顺(不计一色三高,一般高,四归一,一色三通顺)
				{
					result.HuKind.del(HUTYPE_YISE_SANJIEGAO);
					result.HuKind.del(HUTYPE_YIBAN_GAO);
					result.HuKind.del(HUTYPE_SI_GUI_YI);
					result.HuKind.del(HUTYPE_YISE_SANTONGSHUN);
				}break;
			case HUTYPE_YISE_SIJIEGAO:	// 一色四节高(不计一色三通顺,一色三节高,碰碰胡)
				{
					result.HuKind.del(HUTYPE_YISE_SANTONGSHUN);
					result.HuKind.del(HUTYPE_YISE_SANJIEGAO);
					result.HuKind.del(HUTYPE_PENG_PENG);
				}break;
			case HUTYPE_YISE_SIBUGAO:	// 一色四步高(不计一色三步高,老少副,连六)
				{
					result.HuKind.del(HUTYPE_YISE_SANBUGAO);
					result.HuKind.del(HUTYPE_LAOSHAO_FU);
					result.HuKind.del(HUTYPE_LIAN_LIU);
				}break;
			case HUTYPE_SAN_GANG:	// 三杠(不计双明杠,双暗杠,明杠,暗杠)
				{
					result.HuKind.del(HUTYPE_SHUANG_MINGGANG);
					result.HuKind.del(HUTYPE_SHUANG_ANGANG);
					result.HuKind.del(HUTYPE_MING_GANG);
					result.HuKind.del(HUTYPE_AN_GANG);
				}break;
			case HUTYPE_HUN_YAOJIU:	// 混幺九(不计碰碰胡,幺九刻,全带幺)
				{
					result.HuKind.del(HUTYPE_PENG_PENG);
					result.HuKind.del(HUTYPE_YAOJIU_KE);
					result.HuKind.del(HUTYPE_QUANDAIYAO);
				}break;
			case HUTYPE_YISE_SANTONGSHUN:	// 一色三同顺(不计一色三节高,一般高)
				{
					result.HuKind.del(HUTYPE_YISE_SANJIEGAO);
					result.HuKind.del(HUTYPE_YIBAN_GAO);
				}break;
			case HUTYPE_YISE_SANJIEGAO:	// 一色三节高(不计一色三同顺)
				{
					result.HuKind.del(HUTYPE_YISE_SANTONGSHUN);
				}break;
			case HUTYPE_QING_LONG:	// 清龙(不计连六,老少副)
				{
					result.HuKind.del(HUTYPE_LIAN_LIU);
					result.HuKind.del(HUTYPE_LAOSHAO_FU);
				}break;
			case HUTYPE_SAN_ANKE:	// 三暗刻(不计双暗刻)
				{
					result.HuKind.del(HUTYPE_SHUANG_ANKE);
				}break;
			case HUTYPE_SAN_FENGKE:	// 三风刻(不计幺九刻)
				{
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_SHUANG_ANGANG:	// 双暗杠(不计双暗刻,暗杠)
				{
					result.HuKind.del(HUTYPE_SHUANG_ANKE);
					result.HuKind.del(HUTYPE_AN_GANG);
				}break;
			case HUTYPE_BUQIUREN:	// 不求人(不计门清,自摸)+++
				{
					result.HuKind.del(HUTYPE_MEN_QING);
					result.HuKind.del(HUTYPE_ZI_MO);					
					//result.DelRight(CHR_ZI_MO);
				}break;
			case HUTYPE_SHUANG_MINGGANG:	// 双明杠(不计明杠)
				{
					result.HuKind.del(HUTYPE_MING_GANG);
				}break;
			case HUTYPE_QIANG_GANG_HU:	// 胡绝张(不计抢杠胡) --- 改成有抢杠胡不计胡绝张
				{
					result.HuKind.del(HUTYPE_HU_JUEZHANG);
					//result.DelRight(CHR_QIANG_GANG);
				}break;
			case HUTYPE_LI_ZHI:	// 立直(不计报听,门清)
				{
					result.HuKind.del(HUTYPE_BAO_TING);
					result.HuKind.del(HUTYPE_MEN_QING);
				}break;
			case HUTYPE_JIAN_KE:	// 箭刻(不计幺九刻)
				{
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_QUAN_FENGKE:	// 圈风刻(不计幺九刻)
				{
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_MEN_FENGKE:	// 门风刻(不计幺九刻)
				{
					result.HuKind.del(HUTYPE_YAOJIU_KE);
				}break;
			case HUTYPE_SHUANG_JIANKE: // 双箭刻(不计箭刻)
				{
					result.HuKind.del(HUTYPE_JIAN_KE);
				}break;
			case HUTYPE_BIAN_ZHANG:
				{
					result.HuKind.del(HUTYPE_KAN_ZHANG);
				}break;
			case HUTYPE_GANG_FLOWER:
				{
					result.HuKind.del(HUTYPE_ZI_MO);
				}break;
			case HUTYPE_MIAOSHOUHUICHUN:
				{
					result.HuKind.del(HUTYPE_ZI_MO);
				}break;
			default:
				break;
			}
		}
	}

	void CMaJiangLogic::BubbleSort(BYTE cbArr[], int iCount, bool bBig)
	{
		if (cbArr == NULL || 1 >= iCount) {
			return;
		}

		for (int i = 0; i < iCount - 1; ++i)
		{
			for (int j = i + 1; j < iCount; ++j)
			{
				if (bBig)
				{
					if (cbArr[j] > cbArr[i])
					{
						BYTE cbTempBig = cbArr[j];
						cbArr[j] = cbArr[i];
						cbArr[i] = cbTempBig;
					}
				}
				else
				{
					if (cbArr[j] < cbArr[i])
					{
						BYTE cbTempBig = cbArr[j];
						cbArr[j] = cbArr[i];
						cbArr[i] = cbTempBig;
					}
				}
			}
		}
	}

	void CMaJiangLogic::CalcHuCardType(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, const BYTE cbCurrentCard, bool bZimo, stAnalyseItem* pAnalyseItem, stActionOper& oper)
	{
		BYTE cbTempShunZiCard[MAX_WEAVE];
		BYTE cbTempShunZiCount = 0;
		memset(cbTempShunZiCard, 0, sizeof(cbTempShunZiCard));

		BYTE cbTempKeZiCard[MAX_WEAVE];
		BYTE cbTempKeZiCount = 0;
		memset(cbTempKeZiCard, 0, sizeof(cbTempKeZiCard));

		BYTE cbTempGangCard[MAX_WEAVE];
		BYTE cbTempGangCount = 0;
		memset(cbTempGangCard, 0, sizeof(cbTempGangCard));

		BYTE cbTempHuCardIndex[MAX_INDEX];
		memset(cbTempHuCardIndex, 0, sizeof(cbTempHuCardIndex));
		cbTempHuCardIndex[SwitchToCardIndex(pAnalyseItem->cbCardEye)] += 2;

		for (uint8 i = 0; i < MAX_WEAVE; i++)
		{
			if(pAnalyseItem->WeaveKind[i] == ACTION_NULL)continue;
			
			BYTE cbTempCard = pAnalyseItem->cbCenterCard[i];

			if (cbTempCard == 0)
			{
				continue;
			}

			if (cbTempCard < emPOKER_WAN1 || cbTempCard > emPOKER_BAI)
			{
				continue;
			}

			BYTE cbTempIndex = SwitchToCardIndex(cbTempCard);
			if (pAnalyseItem->WeaveKind[i] == ACTION_EAT_LEFT)
			{
				cbTempShunZiCard[cbTempShunZiCount] = pAnalyseItem->cbCenterCard[i];
				cbTempShunZiCount++;

				cbTempHuCardIndex[cbTempIndex]++;
				cbTempHuCardIndex[cbTempIndex + 1]++;
				cbTempHuCardIndex[cbTempIndex + 2]++;
			}
			else if (pAnalyseItem->WeaveKind[i] == ACTION_EAT_CENTER)
			{
				cbTempShunZiCard[cbTempShunZiCount] = pAnalyseItem->cbCenterCard[i] - 1;
				cbTempShunZiCount++;

				cbTempHuCardIndex[cbTempIndex]++;
				cbTempHuCardIndex[cbTempIndex - 1]++;
				cbTempHuCardIndex[cbTempIndex + 1]++;
			}
			else if (pAnalyseItem->WeaveKind[i] == ACTION_EAT_RIGHT)
			{
				cbTempShunZiCard[cbTempShunZiCount] = pAnalyseItem->cbCenterCard[i] - 2;
				cbTempShunZiCount++;

				cbTempHuCardIndex[cbTempIndex]++;
				cbTempHuCardIndex[cbTempIndex - 1]++;
				cbTempHuCardIndex[cbTempIndex - 2]++;
			}
			else if (pAnalyseItem->WeaveKind[i] == ACTION_PENG)
			{
				cbTempKeZiCard[cbTempKeZiCount] = pAnalyseItem->cbCenterCard[i];
				cbTempKeZiCount++;

				cbTempHuCardIndex[cbTempIndex] += 3;
			}
			else if (pAnalyseItem->WeaveKind[i] == ACTION_GANG)
			{
				cbTempGangCard[cbTempGangCount] = pAnalyseItem->cbCenterCard[i];
				cbTempGangCount++;

				cbTempHuCardIndex[cbTempIndex] += 4;
			}
		}

		BYTE cbSortShunZiCard[MAX_WEAVE];
		memcpy(cbSortShunZiCard, cbTempShunZiCard, sizeof(cbSortShunZiCard));
		BubbleSort(cbSortShunZiCard, cbTempShunZiCount);

		BYTE cbSortKeZiCard[MAX_WEAVE];
		memcpy(cbSortKeZiCard, cbTempKeZiCard, sizeof(cbSortKeZiCard));
		BubbleSort(cbSortKeZiCard, cbTempKeZiCount);
	
		if (m_pHostTable->GetHuCardPlayerID() > 0)
		{
			LOG_DEBUG("HuCardGroup - uid:%d,cbCurrentCard:0x%02X,cbCardEye:0x%02X,cbShunZi:%d,cbKeZi:%d,cbGang:%d,cbShunZiCard:0x%02X 0x%02X 0x%02X 0x%02X,cbKeZiCard:0x%02X 0x%02X 0x%02X 0x%02X,cbGangCard:0x%02X 0x%02X 0x%02X 0x%02X,cbSortShunZiCard:0x%02X 0x%02X 0x%02X 0x%02X,cbSortKeZiCard:0x%02X 0x%02X 0x%02X 0x%02X",
				m_pHostTable->GetHuCardPlayerID(), cbCurrentCard, pAnalyseItem->cbCardEye, cbTempShunZiCount, cbTempKeZiCount, cbTempGangCount, cbTempShunZiCard[0], cbTempShunZiCard[1], cbTempShunZiCard[2], cbTempShunZiCard[3],
				cbTempKeZiCard[0], cbTempKeZiCard[1], cbTempKeZiCard[2], cbTempKeZiCard[3], cbTempGangCard[0], cbTempGangCard[1], cbTempGangCard[2], cbTempGangCard[3],
				cbSortShunZiCard[0], cbSortShunZiCard[1], cbSortShunZiCard[2], cbSortShunZiCard[3], cbSortKeZiCard[0], cbSortKeZiCard[1], cbSortKeZiCard[2], cbSortKeZiCard[3]);
		}
		
		// 大四喜(四副风刻(杠))
		bool flag = true;
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			//if (m_pHostTable->GetHuCardPlayerID() > 0)
			//{
			//	LOG_DEBUG("大四喜 - uid:%d,i:%d,WeaveKind:%d,cbCenterCard:0x%02X", m_pHostTable->GetHuCardPlayerID(), i, pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]);
			//}
			if (pAnalyseItem->WeaveKind[i] == ACTION_NULL)
			{
				continue;
			}
			if (pAnalyseItem->WeaveKind[i] != ACTION_PENG && pAnalyseItem->WeaveKind[i] != ACTION_GANG) {
				flag = false;
				break;
			}
			if (!IsFengZi(pAnalyseItem->cbCenterCard[i])) {
				flag = false;
				break;
			}
		}
		if (flag) {
			oper.add(HUTYPE_DA_SI_XI);
		}
		// 大三元(中发白三个刻子)
		flag = true;
		if (IsExistKeZi(emPOKER_ZHONG, pAnalyseItem) && IsExistKeZi(emPOKER_FA, pAnalyseItem) && IsExistKeZi(emPOKER_BAI, pAnalyseItem)) {
			oper.add(HUTYPE_DA_SAN_YUAN);
		}
		// 四杠(4个杠)
		if (cbTempGangCount == 4) {
			oper.add(HUTYPE_SI_GANG);
		}
		// 连七对(由一种花色组成的序数相连的七对) 外面特殊处理 toney

		// 百万石(全部万牌,万总数过100)
		flag = true;
		uint32 countNum = 0;
		for (uint8 i = 0; i<MAX_WEAVE; ++i)
		{
			if (pAnalyseItem->WeaveKind[i] == ACTION_NULL)
			{
				continue;
			}
			if (!IsWanZi(pAnalyseItem->cbCenterCard[i]))
			{
				flag = false;
				break;
			}
			uint32 cbCardValve = GetCardValue(pAnalyseItem->cbCenterCard[i]);
			switch (pAnalyseItem->WeaveKind[i])
			{
			case ACTION_GANG:
			{
				countNum += (4 * cbCardValve);
			}break;
			case ACTION_PENG:
			case ACTION_EAT_CENTER:
			{
				countNum += (3 * cbCardValve);
			}break;
			case ACTION_EAT_LEFT:
			{
				countNum += ((3 * cbCardValve) + 3);
			}break;
			case ACTION_EAT_RIGHT:
			{
				countNum += ((3 * cbCardValve) - 3);
			}break;
			default:
				break;
			}
		}
		countNum += (2*GetCardValue(pAnalyseItem->cbCardEye));

		//LOG_DEBUG("HUTYPE_BAI_WAN_DAN - uid:%d,flag:%d,countNum:%d", m_pHostTable->GetHuCardPlayerID(), flag, countNum);
		if (flag && countNum >= 100) {
			oper.add(HUTYPE_BAI_WAN_DAN);
		}
		// 天胡 外面特殊处理

		// 地胡 外面特殊处理

		// 人胡 外面特殊处理

		// 小四喜(3个风牌刻子及风牌将牌)
		countNum = 0;
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if ((pAnalyseItem->WeaveKind[i] == ACTION_GANG || pAnalyseItem->WeaveKind[i] == ACTION_PENG)
				&& IsFengZi(pAnalyseItem->cbCenterCard[i])) {
				countNum++;
			}
		}
		if (countNum >= 3 && IsFengZi(pAnalyseItem->cbCardEye)) {
			oper.add(HUTYPE_XIAO_SI_XI);
		}
		// 小三元(两个箭牌刻子跟箭牌将)
		countNum = 0;
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if ((pAnalyseItem->WeaveKind[i] == ACTION_GANG || pAnalyseItem->WeaveKind[i] == ACTION_PENG)
				&& IsJianZi(pAnalyseItem->cbCenterCard[i])) {
				countNum++;
			}
		}
		if (countNum >= 2 && IsJianZi(pAnalyseItem->cbCardEye)) {
			oper.add(HUTYPE_XIAO_SAN_YUAN);
		}
		// 字一色(全部字牌刻子字牌将)
		countNum = 0;
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if ((pAnalyseItem->WeaveKind[i] == ACTION_GANG || pAnalyseItem->WeaveKind[i] == ACTION_PENG)
				&& IsZiPai(pAnalyseItem->cbCenterCard[i])) {
				countNum++;
			}
		}
		if (countNum == 4 && IsZiPai(pAnalyseItem->cbCardEye)) {
			oper.add(HUTYPE_ZI_YI_SE);
		}
		// 四暗刻(4组暗刻)
		int iAnGangCount = 0;
		int iAllPengCount = cbTempKeZiCount;
		int iPengCount = 0;
		for (uint8 i = 0; i<cbWeaveCount; ++i)
		{
			if (WeaveItem[i].WeaveKind == ACTION_GANG && WeaveItem[i].cbPublicCard == FALSE)
			{
				iAnGangCount++;
			}
			if (WeaveItem[i].WeaveKind == ACTION_PENG)
			{
				iPengCount++;
			}
		}
		if (bZimo == false)
		{
			for (uint8 i = 0; i < cbTempKeZiCount; i++)
			{
				if (cbTempKeZiCard[i] == pAnalyseItem->cbCardEye)
				{
					iPengCount++;
				}
			}
		}

		int iAnKeCount = (iAllPengCount - iPengCount) + iAnGangCount;
		if (iAnKeCount >= 4) {
			oper.add(HUTYPE_SI_AN_KE);
		}

		// 一色双龙会(5万做将牌,并且有123万和789万的顺子各两组)
		flag = true;
		countNum = 0;
		uint32 countNum1 = 0;
		if (pAnalyseItem->cbCardEye != (emPOKER_WAN1 + 4))
			flag = false;
		for (uint8 i = 0; i<MAX_WEAVE; ++i)
		{
			if (!flag)break;
			if (pAnalyseItem->WeaveKind[i] == ACTION_NULL)
			{
				continue;
			}
			switch (pAnalyseItem->WeaveKind[i])
			{
			case ACTION_EAT_LEFT:
			{
				if (pAnalyseItem->cbCenterCard[i] == emPOKER_WAN1) {
					countNum++;
				}
				else if (pAnalyseItem->cbCenterCard[i] == (emPOKER_WAN1 + 6)) {
					countNum1++;
				}
				else {
					flag = false;
				}
			}break;
			case ACTION_EAT_RIGHT:
			{
				if (pAnalyseItem->cbCenterCard[i] == (emPOKER_WAN1 + 2)) {
					countNum++;
				}
				else if (pAnalyseItem->cbCenterCard[i] == (emPOKER_WAN1 + 8)) {
					countNum1++;
				}
				else {
					flag = false;
				}
			}break;
			case ACTION_EAT_CENTER:
			{
				if (pAnalyseItem->cbCenterCard[i] == (emPOKER_WAN1 + 1)) {
					countNum++;
				}
				else if (pAnalyseItem->cbCenterCard[i] == (emPOKER_WAN1 + 7)) {
					countNum1++;
				}
				else {
					flag = false;
				}
			}break;
			default:
			{
				//flag = false;
			}break;
			}
		}
		if (countNum == 2 && countNum1 == 2 && flag) {
			oper.add(HUTYPE_YISE_SHUANG_LONG);
		}
		// 一色四同顺(4组相同的顺子)

		//flag = true;
		//for (uint8 i = 0; i<MAX_WEAVE; ++i)
		//{
		//	if (pAnalyseItem->WeaveKind[i] == ACTION_NULL)
		//	{
		//		continue;
		//	}
		//	if (!flag)break;
		//	if (pAnalyseItem->WeaveKind[i]<ACTION_EAT_LEFT || pAnalyseItem->WeaveKind[i]>ACTION_EAT_RIGHT) {
		//		flag = false;
		//	}
		//	else {
		//		cards.push_back(GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]));
		//	}
		//}
		//if (flag && cards.size() == 4)
		//{
		//	sort(cards.begin(), cards.end());
		//	if (cards[0] == cards[3])
		//	{
		//		oper.add(HUTYPE_YISE_SITONGSHUN);
		//	}
		//}
		if (cbTempShunZiCount == 4 && IsWanZi(cbTempShunZiCard[0]) && cbTempShunZiCard[0] == cbTempShunZiCard[1] && cbTempShunZiCard[0] == cbTempShunZiCard[2] && cbTempShunZiCard[0] == cbTempShunZiCard[3])
		{
			oper.add(HUTYPE_YISE_SITONGSHUN);
		}
		// 一色四节高(4组递增的刻子)
		if (cbTempKeZiCount == 4 && IsWanZi(cbSortKeZiCard[0]) && cbSortKeZiCard[0] + 1 == cbSortKeZiCard[1] && cbSortKeZiCard[0] + 2 == cbSortKeZiCard[2] && cbSortKeZiCard[0] + 3 == cbSortKeZiCard[3])
		{
			oper.add(HUTYPE_YISE_SIJIEGAO);
		}

		//flag = true;
		//
		//cards.clear();
		//for (uint8 i = 0; i<MAX_WEAVE; ++i) {
		//	if (!flag)break;
		//	BYTE curColor = GetCardColorValue(pAnalyseItem->cbCenterCard[i]);
		//	if (curColor == emPOKER_COLOR_SUO || curColor == emPOKER_COLOR_TONG || curColor == emPOKER_COLOR_ZI)
		//	{
		//		flag = false;
		//	}
		//	if (pAnalyseItem->WeaveKind[i] != ACTION_PENG) {
		//		flag = false;
		//	}
		//	else {
		//		cards.push_back(pAnalyseItem->cbCenterCard[i]);
		//	}
		//}
		//if (flag && cards.size() == MAX_WEAVE) {
		//	sort(cards.begin(), cards.end());
		//	if (abs(cards[0] - cards[3]) == 3) {
		//		oper.add(HUTYPE_YISE_SIJIEGAO);
		//	}
		//}



		// 一色四步高(4组依次递增1位或2位的顺子)

		//flag = true;
		//vector<BYTE> cards;
		//cards.clear();
		//for (uint8 i = 0; i<MAX_WEAVE; ++i) {
		//	if (!flag)break;
		//	if (pAnalyseItem->WeaveKind[i]<ACTION_EAT_LEFT || pAnalyseItem->WeaveKind[i]>ACTION_EAT_RIGHT) {
		//		flag = false;
		//	}
		//	else {
		//		cards.push_back(GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]));
		//	}
		//}
		//if (flag && cards.size() == MAX_WEAVE) {
		//	sort(cards.begin(), cards.end());
		//	if (abs(cards[0] - cards[1]) == 1 && abs(cards[1] - cards[2]) == 1
		//		&& abs(cards[2] - cards[3]) == 1) {
		//		oper.add(HUTYPE_YISE_SIBUGAO);
		//	}
		//	if (abs(cards[0] - cards[1]) == 2 && abs(cards[1] - cards[2]) == 2
		//		&& abs(cards[2] - cards[3]) == 2) {
		//		oper.add(HUTYPE_YISE_SIBUGAO);
		//	}
		//}
		if (cbTempShunZiCount == 4 && cbSortShunZiCard[0] + 1 == cbSortShunZiCard[1] && cbSortShunZiCard[0] + 2 == cbSortShunZiCard[2] && cbSortShunZiCard[0] + 3 == cbSortShunZiCard[3])
		{
			oper.add(HUTYPE_YISE_SIBUGAO);
		}
		if (cbTempShunZiCount == 4 && cbSortShunZiCard[0] + 2 == cbSortShunZiCard[1] && cbSortShunZiCard[0] + 4 == cbSortShunZiCard[2] && cbSortShunZiCard[0] + 6 == cbSortShunZiCard[3])
		{
			oper.add(HUTYPE_YISE_SIBUGAO);
		}

		// 三杠(3个杠牌)
		if (cbTempGangCount >= 3) {
			oper.add(HUTYPE_SAN_GANG);
		}
		// 混幺九(全部为字牌或者1万9万)
		bool bIsHunYaoJiu = true;
		for (uint32 i = 0; i < cbTempKeZiCount; i++)
		{
			if (cbTempKeZiCard[i] != emPOKER_WAN1 && cbTempKeZiCard[i] != emPOKER_WAN1 + 8 && IsZiPai(cbTempKeZiCard[i])==false)
			{
				bIsHunYaoJiu = false;
				break;
			}
		}
		for (uint32 i = 0; i < cbTempGangCount; i++)
		{
			//if (m_pHostTable->GetHuCardPlayerID() > 0)
			//{
			//	LOG_DEBUG("HUTYPE_HUN_YAOJIU 1 - uid:%d,i:%d,cbTempGangCard:0x%02X", m_pHostTable->GetHuCardPlayerID(), i, cbTempGangCard[i]);
			//}
			if (cbTempGangCard[i] != emPOKER_WAN1 && cbTempGangCard[i] != emPOKER_WAN1 + 8 && IsZiPai(cbTempGangCard[i]) == false)
			{
				//if (m_pHostTable->GetHuCardPlayerID() > 0)
				//{
				//	LOG_DEBUG("HUTYPE_HUN_YAOJIU 2 - uid:%d,i:%d,cbTempGangCard:0x%02X", m_pHostTable->GetHuCardPlayerID(), i, cbTempGangCard[i]);
				//}
				bIsHunYaoJiu = false;
				break;
			}
		}
		if (pAnalyseItem->cbCardEye != emPOKER_WAN1 && pAnalyseItem->cbCardEye != emPOKER_WAN1 + 8 && IsZiPai(pAnalyseItem->cbCardEye) == false)
		{
			bIsHunYaoJiu = false;
		}
		
		if (((cbTempKeZiCount + cbTempGangCount) == 4) && bIsHunYaoJiu == true)
		{
			//if (bIsHunYaoJiu)
			//{
			oper.add(HUTYPE_HUN_YAOJIU);
			//}
		}

		//flag = true;
		//for (uint8 i = 0; i<MAX_WEAVE; ++i) {
		//	if (!IsZiPai(pAnalyseItem->cbCenterCard[i])
		//		&& pAnalyseItem->cbCenterCard[i] != emPOKER_WAN1
		//		&& pAnalyseItem->cbCenterCard[i] != emPOKER_WAN1 + 8) {
		//		flag = false;
		//	}
		//	if (pAnalyseItem->WeaveKind[i] != ACTION_PENG
		//		&& pAnalyseItem->WeaveKind[i] != ACTION_GANG) {
		//		flag = false;
		//	}
		//	if (!flag)break;
		//}
		//if (flag) {
		//	oper.add(HUTYPE_HUN_YAOJIU);
		//}

		// 七对 外面特殊处理

		// 清一色 外面特殊处理

		// 一色三同顺(相同的3组顺子)
		vector<BYTE> cards;
		cards.clear();
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if (pAnalyseItem->WeaveKind[i] >= ACTION_EAT_LEFT && pAnalyseItem->WeaveKind[i] <= ACTION_EAT_RIGHT) {
				cards.push_back(GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]));
			}
		}
		if (cards.size() >= 3) {
			sort(cards.begin(), cards.end());
			if (cards[0] == cards[2]) {
				oper.add(HUTYPE_YISE_SANTONGSHUN);
			}
			if (cards.size() == 4 && (cards[0] == cards[2] || cards[1] == cards[3])) {
				oper.add(HUTYPE_YISE_SANTONGSHUN);
			}
		}
		// 一色三节高(3组依次递增一位的刻字)
		cards.clear();
		for (uint8 i = 0; i<MAX_WEAVE; ++i)
		{
			BYTE curColor = GetCardColorValue(pAnalyseItem->cbCenterCard[i]);
			if (curColor == emPOKER_COLOR_SUO || curColor == emPOKER_COLOR_TONG || curColor == emPOKER_COLOR_ZI)
			{
				continue;
			}
			if (pAnalyseItem->WeaveKind[i] == ACTION_PENG )
			{
				cards.push_back(pAnalyseItem->cbCenterCard[i]);
			}
		}
		if (cards.size() >= 3) {
			sort(cards.begin(), cards.end());
			if (abs(cards[0] - cards[2]) == 2) {
				oper.add(HUTYPE_YISE_SANJIEGAO);
			}
			if (cards.size() == 4) {
				if (abs(cards[0] - cards[2]) == 2 || abs(cards[1] - cards[3]) == 2) {
					oper.add(HUTYPE_YISE_SANJIEGAO);
				}
			}
		}
		// 清龙(有1万到9万相连的9张牌) 吃碰的牌是否算? toney
		bool bIsQingLong = true;
		for (uint32 i = 0; i < 9; i++)
		{
			if (0 >= cbTempHuCardIndex[i])
			{
				bIsQingLong = false;
			}
		}
		if (bIsQingLong)
		{
			oper.add(HUTYPE_QING_LONG);
		}
		
		//flag = true;
		//for (uint32 i = 0; i<9; ++i) {
		//	if (cbCardIndex[SwitchToCardIndex(emPOKER_WAN1 + i)] < 1) {
		//		flag = false;
		//		break;
		//	}
		//}
		//if (flag) {
		//	oper.add(HUTYPE_QING_LONG);
		//}
		// 一色三步高
		cards.clear();
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if (pAnalyseItem->WeaveKind[i] >= ACTION_EAT_LEFT && pAnalyseItem->WeaveKind[i] <= ACTION_EAT_RIGHT) {
				cards.push_back(GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]));
			}
		}
		if (cards.size() >= 3) {
			sort(cards.begin(), cards.end());
			if (abs(cards[0] - cards[1]) == 1 && abs(cards[1] - cards[2]) == 1) {
				oper.add(HUTYPE_YISE_SANBUGAO);
			}
			if (abs(cards[0] - cards[1]) == 2 && abs(cards[1] - cards[2]) == 2) {
				oper.add(HUTYPE_YISE_SANBUGAO);
			}
			if (cards.size() == 4 && abs(cards[1] - cards[2]) == 1 && abs(cards[2] - cards[3]) == 1) {
				oper.add(HUTYPE_YISE_SANBUGAO);
			}
			if (cards.size() == 4 && abs(cards[1] - cards[2]) == 2 && abs(cards[2] - cards[3]) == 2) {
				oper.add(HUTYPE_YISE_SANBUGAO);
			}
		}
		// 三暗刻(3个暗刻)
		if (iAnKeCount >= 3) {
			oper.add(HUTYPE_SAN_ANKE);
		}
		
		// 天听  外面特殊处理 toney

		// 大于五(6万到9万)
		flag = true;
		for (uint8 i = 0; i<MAX_WEAVE-1; ++i)
		{
			if (!IsWanZi(pAnalyseItem->cbCenterCard[i])) {
				flag = false;
				break;
			}
			if (pAnalyseItem->WeaveKind[i] == ACTION_PENG || pAnalyseItem->WeaveKind[i] == ACTION_GANG) {
				if (GetCardValue(pAnalyseItem->cbCenterCard[i]) < 6) {
					flag = false;
					break;
				}
				continue;
			}
			if (GetCardValue(GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i])) < 7) {
				flag = false;
				break;
			}
		}
		if (flag && IsWanZi(pAnalyseItem->cbCardEye) && GetCardValue(pAnalyseItem->cbCardEye) > 5) {
			oper.add(HUTYPE_DAYU_WU);
		}
		// 小于五(1万到4万)
		flag = true;
		for (uint8 i = 0; i<MAX_WEAVE-1; ++i)
		{
			if (!IsWanZi(pAnalyseItem->cbCenterCard[i])) {
				flag = false;
				break;
			}
			if (pAnalyseItem->WeaveKind[i] == ACTION_PENG || pAnalyseItem->WeaveKind[i] == ACTION_GANG) {
				if (GetCardValue(pAnalyseItem->cbCenterCard[i]) > 4) {
					flag = false;
					break;
				}
				continue;
			}
			if (GetCardValue(GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i])) > 3) {
				flag = false;
				break;
			}
		}
		if (flag && IsWanZi(pAnalyseItem->cbCardEye) && GetCardValue(pAnalyseItem->cbCardEye) < 5) {
			oper.add(HUTYPE_XIAOYU_WU);
		}
		// 三风刻(3个风刻)
		countNum = 0;
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if ((pAnalyseItem->WeaveKind[i] == ACTION_GANG || pAnalyseItem->WeaveKind[i] == ACTION_PENG)
				&& IsFengZi(pAnalyseItem->cbCenterCard[i])) {
				countNum++;
			}
		}
		if (countNum >= 3) {
			oper.add(HUTYPE_SAN_FENGKE);
		}
		// 妙手回春(自摸牌池最后一张牌)
		if (bZimo && m_pHostTable->GetCardPoolSize() == 0) {
			oper.add(HUTYPE_MIAOSHOUHUICHUN);
		}

		// 海底捞月
		if (!bZimo && m_pHostTable->GetCardPoolSize() == 0)
		{
			oper.add(HUTYPE_HAI_DI_LAO_YUE);
		}
		// 杠上开花 外面特殊处理
		if (bZimo && m_pHostTable->GetOperHuCardIsGangFalwore())
		{
			oper.add(HUTYPE_GANG_FLOWER);
		}
		// 抢杠胡 外面特殊处理

		// 碰碰胡 外面特殊处理

		// 混一色(万牌字牌组成)
		bool bIsHunYiSe = true;
		bool bIsHaveWanCard = false;
		bool bIsHaveZiCard = false;		
		for (uint8 i = 0; i<MAX_WEAVE; ++i)
		{
			if (pAnalyseItem->WeaveKind[i] == ACTION_NULL)
			{
				continue;
			}
			//LOG_DEBUG("HUTYPE_HUN_YISE 1 - uid:%d,i:%d,WeaveKind:%d,cbCenterCard:0x%02X", m_pHostTable->GetHuCardPlayerID(), i, pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]);

			if (IsWanZi(pAnalyseItem->cbCenterCard[i]))
			{
				//LOG_DEBUG("HUTYPE_HUN_YISE  2 - uid:%d,i:%d,WeaveKind:%d,cbCenterCard:0x%02X", m_pHostTable->GetHuCardPlayerID(), i, pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]);

				bIsHaveWanCard = true;
			}
			if (IsZiPai(pAnalyseItem->cbCenterCard[i]))
			{
				bIsHaveZiCard = true;
			}
			if (!IsZiPai(pAnalyseItem->cbCenterCard[i]) && !IsWanZi(pAnalyseItem->cbCenterCard[i]))
			{
				bIsHunYiSe = false;
				break;
			}
		}
		if (IsWanZi(pAnalyseItem->cbCardEye))
		{
			//LOG_DEBUG("HUTYPE_HUN_YISE 3 - uid:%d,bIsHaveZiCard:%d,bIsHaveZiCard:%d,bIsHunYiSe:%d,cbCardEye:0x%02X", m_pHostTable->GetHuCardPlayerID(), bIsHaveZiCard, bIsHaveZiCard, bIsHunYiSe, pAnalyseItem->cbCardEye);

			bIsHaveWanCard = true;
		}
		if (IsZiPai(pAnalyseItem->cbCardEye))
		{
			bIsHaveZiCard = true;
		}
		if (!IsZiPai(pAnalyseItem->cbCardEye) && !IsWanZi(pAnalyseItem->cbCardEye))
		{
			bIsHunYiSe = false;
		}

		//LOG_DEBUG("HUTYPE_HUN_YISE 4 - uid:%d,bIsHaveZiCard:%d,bIsHaveZiCard:%d,bIsHunYiSe:%d,cbCardEye:0x%02X", m_pHostTable->GetHuCardPlayerID(), bIsHaveZiCard, bIsHaveZiCard, bIsHunYiSe, pAnalyseItem->cbCardEye);

		if (bIsHunYiSe && bIsHaveZiCard  && bIsHaveWanCard) {
			oper.add(HUTYPE_HUN_YISE);
		}

		// 全求人 外面特殊处理

		// 双暗杠(不计双暗刻,暗杠)
		countNum = 0;
		for (uint8 i = 0; i<cbWeaveCount; ++i) {
			if (WeaveItem[i].WeaveKind == ACTION_GANG && WeaveItem[i].cbPublicCard == FALSE) {//暗杠
				countNum++;
			}
		}
		if (countNum >= 2) {
			oper.add(HUTYPE_SHUANG_ANGANG);
		}

		// 双箭刻
		countNum = 0;
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if (IsJianZi(pAnalyseItem->cbCenterCard[i]) && (pAnalyseItem->WeaveKind[i] == ACTION_PENG || pAnalyseItem->WeaveKind[i] == ACTION_GANG)) {
				countNum++;
			}
		}
		if (countNum >= 2) {
			oper.add(HUTYPE_SHUANG_JIANKE);
		}

		//flag = true;
		//for (uint8 i = 0; i<MAX_WEAVE; ++i) {
		//	BYTE card = GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]);
		//	if (card != emPOKER_WAN1 && card != emPOKER_WAN1 + 8) {
		//		flag = false;
		//		break;
		//	}
		//}
		//if (pAnalyseItem->cbCardEye != emPOKER_WAN1 && pAnalyseItem->cbCardEye != emPOKER_WAN1 + 8) {
		//	flag = false;
		//}
		//if (flag) {
		//	oper.add(HUTYPE_QUANDAIYAO);
		//}

		// 全带幺(刻字,顺子,将牌都带有1万或者9万)
		bool bIsQuanDaiYao = true;
		for (uint32 i = 0; i < cbTempShunZiCount; i++)
		{
			if (cbTempShunZiCard[i] != emPOKER_WAN1 && cbTempShunZiCard[i] != emPOKER_WAN1 + 6 && IsZiPai(cbTempShunZiCard[i]) == false)
			{
				bIsQuanDaiYao = false;
				break;
			}
		}
		for (uint32 i = 0; i < cbTempKeZiCount; i++)
		{
			if (cbTempKeZiCard[i] != emPOKER_WAN1 && cbTempKeZiCard[i] != emPOKER_WAN1 + 8 && IsZiPai(cbTempKeZiCard[i]) == false)
			{
				bIsQuanDaiYao = false;
				break;
			}
		}
		for (uint32 i = 0; i < cbTempGangCount; i++)
		{
			if (cbTempGangCard[i] != emPOKER_WAN1 && cbTempGangCard[i] != emPOKER_WAN1 + 8 && IsZiPai(cbTempGangCard[i]) == false)
			{
				bIsQuanDaiYao = false;
				break;
			}
		}
		if (pAnalyseItem->cbCardEye != emPOKER_WAN1 && pAnalyseItem->cbCardEye != emPOKER_WAN1 + 8 && IsZiPai(pAnalyseItem->cbCardEye) == false)
		{
			bIsQuanDaiYao = false;
		}

		if (bIsQuanDaiYao)
		{
			oper.add(HUTYPE_QUANDAIYAO);
		}

		// 不求人(没有吃碰杠且自摸胡)
		if (cbWeaveCount == 0 && bZimo) {
			oper.add(HUTYPE_BUQIUREN);
		}
		// 双明杠(不计明杠)
		countNum = 0;
		for (uint8 i = 0; i<cbWeaveCount; ++i) {
			if (WeaveItem[i].WeaveKind == ACTION_GANG && WeaveItem[i].cbPublicCard == TRUE) {//明杠
				countNum++;
			}
		}
		if (countNum >= 2) {
			oper.add(HUTYPE_SHUANG_MINGGANG);
		}
		// 胡绝张(桌面已明亮3张牌,胡所剩第4张)
		if (m_pHostTable->GetShowCardNum(cbCurrentCard) == 3) {
			oper.add(HUTYPE_HU_JUEZHANG);
		}
		// 立直(门前清且听牌后胡牌) 外面特殊处理

		// 箭刻(有1组箭牌刻字)
		for (uint8 i = 0; i<MAX_WEAVE; ++i) {
			if (IsJianZi(pAnalyseItem->cbCenterCard[i])
				&& (pAnalyseItem->WeaveKind[i] == ACTION_PENG || pAnalyseItem->WeaveKind[i] == ACTION_GANG)) {
				oper.add(HUTYPE_JIAN_KE);
			}
		}
		// 圈风刻(有与圈风相同风牌刻子)

		// 门风刻(有与门风相同的风牌刻子)

		// 门前清(没有吃碰明杠) 外面特殊处理

		//平胡 胡牌时，牌型由4组顺子和万字牌做将组成
		if (cbTempShunZiCount == 4 && GetCardColor(pAnalyseItem->cbCardEye) == emPOKER_COLOR_WAN)
		{
			oper.add(HUTYPE_PING_HU);
		}

		// 四归一(有4张相同的牌归于1家的顺子,刻子,将牌,不包括开杠)
		bool bIsSiGuiYi = false;
		for (uint32 i = 0; i < cbTempKeZiCount; i++)
		{
			for (uint32 j = 0; j < cbTempShunZiCount; j++)
			{
				if (cbTempKeZiCard[i] == cbTempShunZiCard[j])
				{
					bIsSiGuiYi = true;
					break;
				}
			}
			if (bIsSiGuiYi)break;
		}
		if (bIsSiGuiYi == false)
		{
			uint32 uJiangCardShunZiCount = 0;

			for (uint8 i = 0; i < MAX_WEAVE; i++)
			{
				if (pAnalyseItem->WeaveKind[i] == ACTION_NULL)continue;

				BYTE cbTempCard = pAnalyseItem->cbCenterCard[i];

				if (cbTempCard < emPOKER_WAN1 || cbTempCard > emPOKER_BAI)
				{
					continue;
				}
				if (pAnalyseItem->WeaveKind[i] == ACTION_EAT_LEFT)
				{
					if (pAnalyseItem->cbCardEye == cbTempCard || pAnalyseItem->cbCardEye == cbTempCard + 1 || pAnalyseItem->cbCardEye == cbTempCard + 2)
					{
						uJiangCardShunZiCount++;
					}
				}
				else if (pAnalyseItem->WeaveKind[i] == ACTION_EAT_CENTER)
				{
					if (pAnalyseItem->cbCardEye == cbTempCard || pAnalyseItem->cbCardEye == cbTempCard - 1 || pAnalyseItem->cbCardEye == cbTempCard + 1)
					{
						uJiangCardShunZiCount++;
					}
				}
				else if (pAnalyseItem->WeaveKind[i] == ACTION_EAT_RIGHT)
				{
					if (pAnalyseItem->cbCardEye == cbTempCard || pAnalyseItem->cbCardEye == cbTempCard - 1 || pAnalyseItem->cbCardEye == cbTempCard - 2)
					{
						uJiangCardShunZiCount++;
					}
				}
			}
			if (uJiangCardShunZiCount == 2)
			{
				bIsSiGuiYi = true;
			}
		}
		if (bIsSiGuiYi)
		{
			oper.add(HUTYPE_SI_GUI_YI);
		}

		
		//BYTE cbProvideUserWeaveCount[GAME_PLAYER];
		//memset(cbProvideUserWeaveCount,0,sizeof(cbProvideUserWeaveCount));
		//for (uint8 i = 0; i < cbWeaveCount; i++)
		//{
		//	LOG_DEBUG("HUTYPE_SI_GUI_YI1 - uid:%d,cbWeaveCount:%d,i:%d,WeaveKind:%d,wProvideUser:%d, cbCenterCard:0x%02X",
		//		m_pHostTable->GetHuCardPlayerID(), cbWeaveCount,i, WeaveItem[i].WeaveKind, WeaveItem[i].wProvideUser, WeaveItem[i].cbCenterCard);
		//	if (WeaveItem[i].WeaveKind == ACTION_PENG && WeaveItem[i].WeaveKind == ACTION_EAT_LEFT && WeaveItem[i].WeaveKind == ACTION_EAT_CENTER && WeaveItem[i].WeaveKind == ACTION_EAT_RIGHT )
		//	{
		//		if (WeaveItem[i].wProvideUser < m_pHostTable->GetPlayerCount())
		//		{
		//			cbProvideUserWeaveCount[WeaveItem[i].wProvideUser]++;
		//		}
		//	}
		//}
		//if (bZimo == false && pAnalyseItem->cbCardEye == cbCurrentCard)
		//{
		//	if (m_pHostTable->GetProvideUser() < m_pHostTable->GetPlayerCount())
		//	{
		//		cbProvideUserWeaveCount[m_pHostTable->GetProvideUser()]++;
		//	}
		//}
		//for (uint32 i = 0; i < m_pHostTable->GetPlayerCount(); i++)
		//{
		//	if (cbProvideUserWeaveCount[i]>=4)
		//	{
		//		oper.add(HUTYPE_SI_GUI_YI);
		//	}
		//}
		//LOG_DEBUG("HUTYPE_SI_GUI_YI2 - uid:%d,bZimo:%d,PlayerCount:%d,cbCardEye:0x%02X,cbCurrentCard:0x%02X,cbProvideUserWeaveCount:%d %d %d %d", m_pHostTable->GetHuCardPlayerID(), bZimo, m_pHostTable->GetPlayerCount(),
		//	pAnalyseItem->cbCardEye, cbCurrentCard,cbProvideUserWeaveCount[0], cbProvideUserWeaveCount[1], cbProvideUserWeaveCount[2], cbProvideUserWeaveCount[3]);

		// 双暗刻
		if (iAnKeCount >= 2) {
			oper.add(HUTYPE_SHUANG_ANKE);
		}
		// 暗杠(一个暗杠)
		countNum = 0;
		for (uint8 i = 0; i<cbWeaveCount; ++i) {
			if (WeaveItem[i].WeaveKind == ACTION_GANG && WeaveItem[i].cbPublicCard == FALSE) {//暗杠
				countNum++;
			}
		}
		if (countNum >= 1) {
			oper.add(HUTYPE_AN_GANG);
		}
		// 断幺九(没有1 9万及字牌)
		bool bIsDuanYaoJiu = true;
		for (uint32 i = 0; i < cbTempShunZiCount; i++)
		{
			if (cbTempShunZiCard[i] == emPOKER_WAN1 || cbTempShunZiCard[i] == emPOKER_WAN1 + 8 || IsZiPai(cbTempShunZiCard[i]))
			{
				bIsDuanYaoJiu = false;
				break;
			}
		}
		if (bIsDuanYaoJiu)
		{
			for (uint32 i = 0; i < cbTempKeZiCount; i++)
			{
				if (cbTempKeZiCard[i] == emPOKER_WAN1 || cbTempKeZiCard[i] == emPOKER_WAN1 + 8 || IsZiPai(cbTempKeZiCard[i]))
				{
					bIsDuanYaoJiu = false;
					break;
				}
			}
		}
		if (bIsDuanYaoJiu)
		{
			for (uint32 i = 0; i < cbTempGangCount; i++)
			{
				if (cbTempGangCard[i] != emPOKER_WAN1 || cbTempGangCard[i] != emPOKER_WAN1 + 8 || IsZiPai(cbTempGangCard[i]))
				{
					bIsDuanYaoJiu = false;
					break;
				}
			}
		}
		if (bIsDuanYaoJiu)
		{
			if (pAnalyseItem->cbCardEye != emPOKER_WAN1 || pAnalyseItem->cbCardEye != emPOKER_WAN1 + 8 || IsZiPai(pAnalyseItem->cbCardEye))
			{
				bIsDuanYaoJiu = false;
			}
		}
		if (bIsDuanYaoJiu)
		{
			oper.add(HUTYPE_DUAN_YAOJIU);
		}


		//flag = true;
		//for (uint8 i = 0; i<MAX_WEAVE; ++i) {
		//	BYTE card;
		//	if (pAnalyseItem->WeaveKind[i] == ACTION_PENG || pAnalyseItem->WeaveKind[i] == ACTION_GANG) {
		//		card = pAnalyseItem->cbCenterCard[i];
		//	}
		//	else {
		//		card = GetEatCenterCard(pAnalyseItem->WeaveKind[i], pAnalyseItem->cbCenterCard[i]);
		//	}
		//	if (card == emPOKER_WAN1 + 1 || card == emPOKER_WAN1 + 7 || IsZiPai(card)) {
		//		flag = false;
		//		break;
		//	}
		//}
		//if (flag) {
		//	oper.add(HUTYPE_DUAN_YAOJIU);
		//}

		// 二五八将(将牌是258万)
		if (pAnalyseItem->cbCardEye == emPOKER_WAN1 + 1 || pAnalyseItem->cbCardEye == emPOKER_WAN1 + 4
			|| pAnalyseItem->cbCardEye == emPOKER_WAN1 + 7) {
			oper.add(HUTYPE_ERWUBA_JIANG);
		}
		// 幺九头(将牌是19万)
		if (pAnalyseItem->cbCardEye == emPOKER_WAN1 || pAnalyseItem->cbCardEye == emPOKER_WAN1 + 8) {
			oper.add(HUTYPE_YAOJIU_TOU);
		}
		// 报听(听牌后胡牌)
		if (m_pHostTable->GetOperHuCardIsListen())
		{
			oper.add(HUTYPE_BAO_TING);
		}

		// 一般高(有相同的两组顺子)
		bool isHaveSameShunZi = false;
		if (cbTempShunZiCount >= 2)
		{
			for (uint8 i = 0; i < cbTempShunZiCount - 1; i++)
			{
				for (uint8 j = 0; j < cbTempShunZiCount; j++)
				{
					if (i == j)
					{
						continue;
					}
					if (cbTempShunZiCard[i] == cbTempShunZiCard[j])
					{
						isHaveSameShunZi = true;
					}
				}
			}
		}
		if (isHaveSameShunZi == true)
		{
			oper.add(HUTYPE_YIBAN_GAO);
		}

		// 连六(6张数字相连的万字牌)
		bool bIsLianWanSix = false;
		for (uint32 i = 0; i <= 3; i++)
		{
			//LOG_DEBUG("uid:%d,i:%d,cbTempHuCardIndex:%d,"m_pHostTable->GetHuCardPlayerID(),i, cbTempHuCardIndex[i]);
			if (cbTempHuCardIndex[i] <= 0)
			{
				continue;
			}
			uint32 j = i + 1;
			for (; j <= i+5; j++)
			{
				if (cbTempHuCardIndex[j] <= 0)
				{
					break;
				}
			}
			//LOG_DEBUG("HUTYPE_LIAN_LIU uid:%d,i:%d,j:%d,cbTempHuCardIndex:%d",m_pHostTable->GetHuCardPlayerID(), i,j, cbTempHuCardIndex[i]);

			if (j-1 == i + 5)
			{
				bIsLianWanSix = true;
				break;
			}
		}
		if (bIsLianWanSix == true)
		{
			oper.add(HUTYPE_LIAN_LIU);
		}
		// 老少副(123,789w顺子各一组)
		bool bIsLaoShaoFuWan1 = false;
		bool bIsLaoShaoFuWan7 = false;
		for (uint8 i = 0; i < cbTempShunZiCount; i++)
		{
			if (cbTempShunZiCard[i]== emPOKER_WAN1)
			{
				bIsLaoShaoFuWan1 = true;
			}
			if (cbTempShunZiCard[i] == emPOKER_WAN1+6)
			{
				bIsLaoShaoFuWan7 = true;
			}
		}
		if (bIsLaoShaoFuWan1 && bIsLaoShaoFuWan7)
		{
			oper.add(HUTYPE_LAOSHAO_FU);
		}


		// 幺九刻(1,9万或字牌刻子)
		for (uint8 i = 0; i<cbTempKeZiCount; ++i)
		{
			if (IsZiPai(cbTempKeZiCard[i]) || cbTempKeZiCard[i] == emPOKER_WAN1 || cbTempKeZiCard[i] == emPOKER_WAN1 + 8)
			{
				oper.add(HUTYPE_YAOJIU_KE);
			}
		}
		
		//for (uint8 i = 0; i<MAX_WEAVE; ++i) {
		//	if (pAnalyseItem->WeaveKind[i] == ACTION_GANG || pAnalyseItem->WeaveKind[i] == ACTION_PENG) {
		//		BYTE card = pAnalyseItem->cbCenterCard[i];
		//		if (card == emPOKER_WAN1 || card == emPOKER_WAN1 + 8 || IsZiPai(card)) {
		//			oper.add(HUTYPE_YAOJIU_KE);
		//			break;
		//		}
		//	}
		//}

		// 明杠(1明杠)
		countNum = 0;
		for (uint8 i = 0; i<cbWeaveCount; ++i) {
			if (WeaveItem[i].WeaveKind == ACTION_GANG && WeaveItem[i].cbPublicCard == TRUE) {//明杠
				countNum++;
			}
		}
		if (countNum >= 1) {
			oper.add(HUTYPE_MING_GANG);
		}
		// 边张(单胡123的3,789的7都为边张,12345胡3不算边张)
		bool bIsBianZhangWan3 = false;
		bool bIsBianZhangWan7 = false;
		for (uint8 i = 0; i < cbTempShunZiCount; i++)
		{
			if (cbTempShunZiCard[i] == emPOKER_WAN1 && cbCurrentCard== emPOKER_WAN1+2)
			{
				bIsBianZhangWan3 = true;
			}
			if (cbTempShunZiCard[i] == emPOKER_WAN1+6 && cbCurrentCard == emPOKER_WAN1 + 6)
			{
				bIsBianZhangWan7 = true;
			}
		}
		if (m_pHostTable->GetOperPlayerHuCardType(cbCurrentCard) == false)
		{
			bIsBianZhangWan3 = false;
			bIsBianZhangWan7 = false;
		}
		if (bIsBianZhangWan3 || bIsBianZhangWan7)
		{
			oper.add(HUTYPE_BIAN_ZHANG);
		}

		// 坎张(胡2张之间的牌,4556胡5也算坎张,45567胡6不算坎张)
		bool bIsKanZhang = false;
		for (uint8 i = 0; i < cbTempShunZiCount; i++)
		{
			if (cbTempShunZiCard[i] +1 == cbCurrentCard)
			{
				bIsKanZhang = true;
				break;
			}
		}
		if (m_pHostTable->GetOperPlayerHuCardType(cbCurrentCard) == false)
		{
			bIsKanZhang = false;
		}
		if (bIsKanZhang)
		{
			oper.add(HUTYPE_KAN_ZHANG);
		}

		// 单调将(胡单张牌,并且为将牌)
		if (cbCurrentCard == pAnalyseItem->cbCardEye && m_pHostTable->GetOperPlayerHuCardType(cbCurrentCard)) {
			oper.add(HUTYPE_DANDIAO_JIANG);
		}

		// 自摸 外面特殊处理

	}



	// 判断是否存在相应刻子	
	bool CMaJiangLogic::IsExistKeZi(BYTE card,stAnalyseItem* pAnalyseItem)
	{
		for(uint8 i=0;i<MAX_WEAVE;++i){
			if(pAnalyseItem->cbCenterCard[i] == card && (pAnalyseItem->WeaveKind[i] == ACTION_PENG
			|| pAnalyseItem->WeaveKind[i] == ACTION_GANG)){
				return true;
			}
		}
		return false;
	}
	bool CMaJiangLogic::IsExistGang(BYTE card,stAnalyseItem* pAnalyseItem)
	{
		for(uint8 i=0;i<MAX_WEAVE;++i){
			if(pAnalyseItem->cbCenterCard[i] == card && pAnalyseItem->WeaveKind[i] == ACTION_GANG){
				return true;
			}
		}
		return false;
	}
	bool CMaJiangLogic::IsWanZi(BYTE card)
	{
		if(card >= emPOKER_WAN1 && card <= emPOKER_WAN1+8)
			return true;
		return false;
	}
	bool CMaJiangLogic::IsSuoZi(BYTE card)
	{
		if(card >= emPOKER_SUO1 && card <= emPOKER_SUO1+8)
			return true;
		return false;
	}
	bool CMaJiangLogic::IsTongZi(BYTE card)
	{
		if(card >= emPOKER_TONG1 && card <= emPOKER_TONG1+8)
			return true;
		return false;
	}
	bool CMaJiangLogic::IsFengZi(BYTE card)
	{
		if(card >= emPOKER_DONG && card <= emPOKER_BEI)
			return true;
		return false;
	}
	bool CMaJiangLogic::IsJianZi(BYTE card)
	{
		if(card >= emPOKER_ZHONG && card <= emPOKER_BAI)
			return true;
		return false;
	}
	bool CMaJiangLogic::IsZiPai(BYTE card)
	{
		if(IsFengZi(card) || IsJianZi(card))
			return true;
		return false;
	}
	BYTE CMaJiangLogic::GetEatCenterCard(DWORD action,BYTE centerCard)
	{
		if(action == ACTION_EAT_LEFT){
			return centerCard+1;
		}else if(action == ACTION_EAT_RIGHT){
			return centerCard-1;
		}
		return centerCard;
	}


    //------------------------------------特殊胡牌---------------------------------------------------------------------------------------------------
    // 起手4红中
    bool CMaJiangLogic::IsHongZhongHu(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,BYTE cbCurrentCard)
    {
        if(cbCardIndex[SwitchToCardIndex(emPOKER_ZHONG)] == 4 && cbWeaveCount == 0){
			LOG_DEBUG("起手4红中");
            return true;
        }
		if(cbCurrentCard == emPOKER_ZHONG){
			if(cbCardIndex[SwitchToCardIndex(emPOKER_ZHONG)] == 3 && cbWeaveCount == 0){
				LOG_DEBUG("起手4红中");
				return true;
			}
		}

        return false;
    }
    // 将将胡(手上全部是2、5、8，无需成牌)
    bool CMaJiangLogic::IsJiangJiangHu(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount)
    {
        for(BYTE i=0;i<MAX_INDEX;++i)
        {
            if(cbCardIndex[i] > 0) {
                BYTE value = GetCardValue(SwitchToCardData(i));
                if (value % 3 != 2)
                    return false;
            }
        }
        for(BYTE i=0;i<cbWeaveCount;++i){
            BYTE value = GetCardValue(WeaveItem[i].cbCenterCard);
            if(value%3 != 2)
                return false;
        }
		for(BYTE i=0;i<cbWeaveCount;++i){
			if(WeaveItem[i].WeaveKind != ACTION_PENG && WeaveItem[i].WeaveKind != ACTION_GANG){
				return false;
			}
		}
		LOG_DEBUG("将将胡");
        return true;
    }






};






