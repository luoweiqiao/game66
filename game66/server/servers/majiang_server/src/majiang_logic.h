
#ifndef LAND_LOGIC_HEAD_FILE
#define LAND_LOGIC_HEAD_FILE


#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

class CMaJiangCfg;
class CGameMaJiangTable;

namespace game_majiang
{

//数目定义
static const int  GAME_PLAYER  		= 4;                                  // 麻将游戏人数
static const int  MAX_COUNT    		= 14;                                 // 手牌最大数目

static const int  MAX_WEAVE	   		= 5;								  //最大组合
static const int  MAX_INDEX	   		= 34;								  //最大索引
static const int  MAX_OPER_COUNT	= 80;								  //最大操作类型

	//特殊牌标记
	enum emPOKER_FLAG
	{
		emPOKER_COLOR_WAN 	= 0,
		emPOKER_COLOR_SUO 	= 1,
		emPOKER_COLOR_TONG	= 2,
		emPOKER_COLOR_ZI    = 3,


		emPOKER_WAN1		= 0x01,
		emPOKER_SUO1 		= 0x11,
		emPOKER_TONG1		= 0x21,
		emPOKER_DONG		= 0x31,
		emPOKER_NAN			= 0x32,
		emPOKER_XI			= 0x33,
		emPOKER_BEI			= 0x34,
		emPOKER_ZHONG		= 0x35,
		emPOKER_FA		    = 0x36,
		emPOKER_BAI			= 0x37,

	};


//////////////////////////////////////////////////////////////////////////////////
	enum emMAJIANG_TYPE
	{
		MAJIANG_TYPE_NULL			= 0,
		MAJIANG_TYPE_CHANGSHA 		= 1,	// 长沙麻将
		MAJIANG_TYPE_ZHUANZHUAN		= 2,	// 转转麻将
		MAJIANG_TYPE_NINGXIANG		= 3,	// 宁乡麻将
		MAJIANG_TYPE_HONGZHONG		= 4,	// 红中麻将
		MAJIANG_TYPE_GUOBIAO		= 5,	// 国标麻将
		MAJIANG_TYPE_TWO_PEOPLE		= 6,	// 二人麻将
	};
	//动作定义

	//动作标志
	enum emACTION_TYPE
	{
		ACTION_NULL  		= 0,	 // 没有类型
		ACTION_EAT_LEFT 	= 1<<1,	 // 左吃类型
		ACTION_EAT_CENTER   = 1<<2,  // 中吃类型
		ACTION_EAT_RIGHT	= 1<<3,  // 右吃类型
		ACTION_PENG			= 1<<4,  // 碰牌类型
		ACTION_GANG			= 1<<5,  // 杠牌类型
		ACTION_LISTEN		= 1<<6,  // 听牌类型
		ACTION_CHI_HU		= 1<<7,  // 吃胡类型
		ACTION_QIPAI_HU		= 1<<8,  // 起手胡
		ACTION_NEED_TAIL	= 1<<9,  // 要海底
		ACTION_GANG_TING	= 1<<10, // 杠听(仅长沙麻将)
		ACTION_MING_GANG	= 1<<11, // 明杠类型
		ACTION_AN_GANG		= 1<<12, // 暗杠类型
		ACTION_BU_GANG		= 1<<13, // 补杠类型

	};

	enum emOPERATE_CODE
	{
		OPERATE_CODE_NULL = 0,		// 没有类型
		OPERATE_CODE_OUT_CARD,		// 出牌类型
		OPERATE_CODE_EAT_LEFT,		// 左吃类型
		OPERATE_CODE_EAT_CENTER,	// 中吃类型
		OPERATE_CODE_EAT_RIGHT,		// 右吃类型
		OPERATE_CODE_PENG,			// 碰牌类型
		OPERATE_CODE_MING_GANG,		// 明杠类型
		OPERATE_CODE_AN_GANG,		// 暗杠类型
		OPERATE_CODE_SEND_CARD,		// 发牌类型
		OPERATE_CODE_LISTEN,		// 听牌类型
		OPERATE_CODE_CHI_HU,		// 吃胡类型
		OPERATE_CODE_HU_TYPE,		// 胡牌类型
		OPERATE_CODE_HU_SCORE,		// 胡牌分数
		OPERATE_CODE_LAW,			// 非法类型
	};
	enum emHAND_CARD_PSO
	{
		HAND_CARD_PSO_GAME_START = 0,
		HAND_CARD_PSO_GAME_END,
	};


	//胡牌定义
	//小胡类型
	enum emMIN_HU_TYPE
	{
		MINHU_SI_XI			= 1,	// 小四喜
		MINHU_BANBAN_HU		= 2, 	// 板板胡
		MINHU_QUE_YI_SE		= 3,	// 缺一色
		MINHU_LIULIU_SHUN	= 4,	// 六六顺
		MINHU_JIE_JIE_GAO   = 5, 	// 节节高
		MINHU_SAN_TONG		= 6, 	// 三同
		MINHU_YIZHIHUA		= 7, 	// 一枝花
		MINHU_JINTONG_YUNV  = 8,	// 金童玉女
		MINHU_YIDIANHONG	= 9,	// 一点红

		MINHU_MAX_TYPE,
	};
	//大胡类型
	enum emHU_TYPE
	{
		HUTYPE_NULL				= 0,		// 非胡
		HUTYPE_JI_HU			= 1,		// 基本胡
		HUTYPE_THIRTEEN			= 2,		// 13乱
		HUTYPE_JIANG_JIANG  	= 3,		// 将将胡
		HUTYPE_HONGZHONG		= 4,		// 起手4红中
		HUTYPE_SUPER_QI_DUI 	= 5,		// 豪华七小对
		HUTYPE_SUPER2_QI_DUI	= 6,		// 双豪华七小对
		HUTYPE_SUPER3_QI_DUI	= 7,		// 三豪华七小对

		/*----- 国标麻将番种-------------------------------------*/
		// 88 (番) - 8 (种)
		HUTYPE_DA_SI_XI			= 8,		// 大四喜(不计圈风刻,门风刻,碰碰胡,幺九刻)
		HUTYPE_DA_SAN_YUAN		= 9,		// 大三元(不计双箭刻,箭刻)
		HUTYPE_SI_GANG			= 10,		// 四杠()
		HUTYPE_LIAN_QI_DUI		= 11,		// 连七对(不计七对,清一色,门清,单调) --
		HUTYPE_BAI_WAN_DAN		= 12,		// 百万石(不计清一色)
		HUTYPE_TIAN_HU			= 13,		// 天胡 --
		HUTYPE_DI_HU			= 14,		// 地胡 --
		HUTYPE_REN_HU			= 15,		// 人胡 --

		// 64 (番) - 5 (种)
		HUTYPE_XIAO_SI_XI		= 16,		// 小四喜(不计三风刻)
		HUTYPE_XIAO_SAN_YUAN	= 17,		// 小三元(不计双箭刻,箭刻,幺九刻)
		HUTYPE_ZI_YI_SE			= 18,		// 字一色(不计碰碰胡,混幺九,全带幺,幺九刻)
		HUTYPE_SI_AN_KE			= 19,		// 四暗刻(不计门清,碰碰胡,三暗刻,双安刻,不求人)
		HUTYPE_YISE_SHUANG_LONG	= 20,		// 一色双龙会(不计平胡,七对,清一色,一般高,老少副)

		// 48 (番) - 2 (种)
		HUTYPE_YISE_SITONGSHUN	= 21,		// 一色四同顺(不计一色三高,一般高,四归一,一色三通顺)
		HUTYPE_YISE_SIJIEGAO	= 22,		// 一色四节高(不计一色三通顺,一色三节高,碰碰胡)

		// 32 (番) - 3 (种)
		HUTYPE_YISE_SIBUGAO		= 23,		// 一色四步高(不计一色三步高,老少副,连六)
		HUTYPE_SAN_GANG			= 24,		// 三杠(不计双明杠,双暗杠,明杠,暗杠)
		HUTYPE_HUN_YAOJIU		= 25,		// 混幺九(不计碰碰胡,幺九刻,全带幺)

		// 24 (番) - 4 (种)
		HUTYPE_QI_DUI			= 26,		// 七对 --
		HUTYPE_QING_YI_SE		= 27,		// 清一色 --
		HUTYPE_YISE_SANTONGSHUN	= 28,		// 一色三同顺(不计一色三节高,一般高)
		HUTYPE_YISE_SANJIEGAO	= 29,		// 一色三节高(不计一色三同顺)

		// 16 (番) - 4 (种)
		HUTYPE_QING_LONG		= 30,		// 清龙(不计连六,老少副)
		HUTYPE_YISE_SANBUGAO	= 31,		// 一色三步高
		HUTYPE_SAN_ANKE			= 32,		// 三暗刻(不计双暗刻)
		HUTYPE_TIAN_TING		= 33,		// 天听 --

		// 12 (番) - 3 (种)
		HUTYPE_DAYU_WU			= 34,		// 大于五
		HUTYPE_XIAOYU_WU		= 35,		// 小于五
		HUTYPE_SAN_FENGKE		= 36,		// 三风刻(不计幺九刻)

		// 8 (番) - 4 (种)
		HUTYPE_MIAOSHOUHUICHUN	= 37,		// 妙手回春
		HUTYPE_HAI_DI_LAO_YUE	= 38,		// 海底捞月 --
		HUTYPE_GANG_FLOWER		= 39,		// 杠上开花 --
		HUTYPE_QIANG_GANG_HU	= 40,		// 抢杠胡 --

		// 6 (番) - 5 (种)
		HUTYPE_PENG_PENG		= 41,		// 碰碰胡 --
		HUTYPE_HUN_YISE			= 42,		// 混一色
		HUTYPE_QUAN_QIU_REN		= 43,		// 全求人(不计单调将) --
		HUTYPE_SHUANG_ANGANG	= 44,		// 双暗杠(不计双暗刻,暗杠)
		HUTYPE_SHUANG_JIANKE	= 45,		// 双箭刻

		// 4 (番) - 5 (种)
		HUTYPE_QUANDAIYAO		= 46,		// 全带幺
		HUTYPE_BUQIUREN			= 47,		// 不求人(不计门清,自摸)
		HUTYPE_SHUANG_MINGGANG	= 48,		// 双明杠(不计明杠)
		HUTYPE_HU_JUEZHANG		= 49,		// 胡绝张(不计抢杠胡)
		HUTYPE_LI_ZHI			= 50,		// 立直(不计报听,门清) --

		// 2 (番) - 9 (种)
		HUTYPE_JIAN_KE			= 51,		// 箭刻(不计幺九刻)
		HUTYPE_QUAN_FENGKE		= 52,		// 圈风刻(不计幺九刻) --
		HUTYPE_MEN_FENGKE		= 53,		// 门风刻(不计幺九刻) --
		HUTYPE_MEN_QING			= 54,		// 门前清(没有吃碰明杠) --
		HUTYPE_PING_HU			= 55,		// 平胡 --
		HUTYPE_SI_GUI_YI		= 56,		// 四归一 --
		HUTYPE_SHUANG_ANKE		= 57,		// 双暗刻
		HUTYPE_AN_GANG			= 58,		// 暗杠
		HUTYPE_DUAN_YAOJIU		= 59,		// 断幺九

		// 1 (番) - 12 (种)
		HUTYPE_ERWUBA_JIANG		= 60,		// 二五八将
		HUTYPE_YAOJIU_TOU		= 61,		// 幺九头
		HUTYPE_BAO_TING			= 62,		// 报听 --
		HUTYPE_YIBAN_GAO		= 63,		// 一般高 --
		HUTYPE_LIAN_LIU			= 64,		// 连六 --
		HUTYPE_LAOSHAO_FU		= 65,		// 老少副 --
		HUTYPE_YAOJIU_KE		= 66,		// 幺九刻
		HUTYPE_MING_GANG		= 67,		// 明杠
		HUTYPE_BIAN_ZHANG		= 68,		// 边张 --
		HUTYPE_KAN_ZHANG		= 69,		// 坎张 --
		HUTYPE_DANDIAO_JIANG	= 70,		// 单调将
		HUTYPE_ZI_MO			= 71,		// 自摸 --

		HUTYPE_MAX_TYPE			= 72,		// 最大类型
	};

	//胡牌权位
	enum emHUPAI_RIGHT
	{
		CHR_ZI_MO					= 1<<1,								//自摸
		CHR_QIANG_GANG				= 1<<2,								//抢杠胡
		CHR_GANG_FLOWER				= 1<<3,								//杠上开花
		CHR_TIAN					= 1<<4,								//天胡(庄家上手胡)
		CHR_DI						= 1<<5,								//地胡(闲家上手胡)
		CHR_GANG_SHANG_PAO			= 1<<6,								//杠上炮
		CHR_HAIDILAOYUE				= 1<<7,								//海底捞月
		CHR_QIANG_HAIDI				= 1<<8,								//抢海底
		CHR_RENHU					= 1<<9,								//人胡
		CHR_TIAN_TING				= 1<<10,							//天听
		CHR_LI_ZHI					= 1<<11,							//立直
	};

	//类型子项
	struct stKindItem
	{
		DWORD							WeaveKind;							//组合类型
		BYTE							cbCenterCard;						//中心扑克
		BYTE							cbCardIndex[3];						//扑克索引
	};
	//组合子项
	struct stWeaveItem
	{
		DWORD	WeaveKind;						//组合类型
		BYTE	cbCenterCard;					//中心扑克
		BYTE	cbPublicCard;					//公开标志
		WORD	wProvideUser;					//供应用户
	};

	//组合子项
	struct stTempWeaveItem
	{
		DWORD	WeaveKind;						//组合类型
		BYTE	cbCenterCard;					//中心扑克
		BYTE	cbPublicCard;					//公开标志
		WORD	wProvideUser;					//供应用户
		WORD	wOperateUser;					//操作用户
		void Init()
		{
			WeaveKind = ACTION_NULL;
			cbCenterCard = 0;
			cbPublicCard = 0;
			wProvideUser = INVALID_CHAIR;
			wOperateUser = INVALID_CHAIR;
		}
	};

	//胡牌类型集合
	struct stActionOper
	{
		BYTE cbActions[MAX_OPER_COUNT];
		BYTE cbScore[MAX_OPER_COUNT];
		stActionOper(){
			memset(this,0, sizeof(stActionOper));
		}
		void reset(){
			memset(this,0, sizeof(stActionOper));
		}
		bool isExist(uint8 oper){
			return cbActions[oper] == 1;
		}
		void add(uint8 oper){
			cbActions[oper] = 1;
		}
		void del(uint8 oper){
			cbActions[oper] = 0;
		}
		bool isNull(){
			for(uint8 i=0;i<MAX_OPER_COUNT;++i){
				if(cbActions[i] == 1)return false;
			}
			return true;
		}
		bool isOnlyType(uint8 oper){
			for(uint8 i=0;i<MAX_OPER_COUNT;++i)
			{
				if(cbActions[i] == 1 && i != oper)return false;
			}
			return true;
		}

		void getAllActions(vector<BYTE>& actions){
			actions.clear();
			for(uint8 i=0;i<MAX_OPER_COUNT;++i){
				if(cbActions[i] == 1){
					actions.push_back(i);
				}
			}
		}
		void setScore(uint8 oper, uint8 score) {
			cbScore[oper] = score;
		}
		void getAllTypeFan(vector<BYTE>& score) {
			score.clear();
			for (uint8 i = 0; i<MAX_OPER_COUNT; ++i) {
				if (cbActions[i] == 1) {
					score.push_back(cbScore[i]);
				}
			}
		}
	};

	//胡牌结果
	struct stChiHuResult
	{
		DWORD			dwChiHuRight;					//胡牌权位
		BYTE			ChiHuCard;						//吃胡的牌
		stActionOper	HuKind;							//吃胡类型
		bool IsKind(BYTE kind){
			return HuKind.isExist(kind);
		}
		bool IsRight(DWORD right){
			return (dwChiHuRight&right) != 0;
		}
		void AddRight(DWORD right){
			dwChiHuRight |= right;
		}
		void DelRight(DWORD right){
			dwChiHuRight &= ~right;
		}
		bool IsHu(){
			return (!HuKind.isNull() || dwChiHuRight != 0);
		}
		stChiHuResult(){
			memset(this,0,sizeof(stChiHuResult));
			HuKind.reset();
		}
	};

	//杠牌结果
	struct stGangCardResult
	{
		BYTE	cbCardCount;						//扑克数目
		BYTE	cbCardData[4];						//扑克数据
		BYTE	cbGangType[4];						//杠牌类型
		stGangCardResult()
		{
			Init();
		}
		void Init()
		{
			cbCardCount = 0;
			for (int i = 0; i < 4; i++)
			{
				cbCardData[i] = 0;
				cbGangType[i] = 0;
			}
		}
	};

	//分析子项
	struct stAnalyseItem
	{
		BYTE							cbCardEye;							//牌眼扑克
		DWORD							WeaveKind[MAX_WEAVE];				//组合类型
		BYTE							cbCenterCard[MAX_WEAVE];			//中心扑克
	};

	struct tagAnalyseTingNotifyHu
	{
		BYTE	cbCard;
		uint32	score;
		BYTE	cbCount;
		stChiHuResult chiHuResult;
	};

	//游戏逻辑类
	class CMaJiangLogic : public CPokerLogic
	{
		//变量定义
	protected:
		vector<BYTE>		m_kingCardIndexs;		//王牌
		CMaJiangCfg*		m_pMjCfg;				//麻将配置
		CGameMaJiangTable*  m_pHostTable;			//麻将桌子

		//函数定义
	public:
		//构造函数
		CMaJiangLogic();

		//析构函数
		virtual ~CMaJiangLogic();

		void SetMjCfg(CMaJiangCfg* pCfg);
		void SetMjTable(CGameMaJiangTable* pTable);

		//获取动作名称
		string  GetOperCodeName(uint32 operCode);
		//获取牌的名称
		string  GetCardName(BYTE card);
		//打印牌型
		string  PrintHuType(const stAnalyseItem& item);
		//打印起手胡
		string  PrintOpenHuType(stActionOper& oper);
		//打印大胡
		string  PrintHuKindType(stActionOper& oper);
		//打印权位
		string  PrintHuRightType(uint32 huRight);

	public:
		//删除扑克
		bool RemoveCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbRemoveCard);
		//删除扑克
		bool RemoveCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbRemoveCard[], BYTE cbRemoveCount);
		//删除扑克
		bool RemoveCard(BYTE cbCardData[], BYTE cbCardCount, BYTE cbRemoveCard[], BYTE cbRemoveCount);
		//设置精牌
		void SetKingCardIndex(BYTE cbKingCardIndex);
		//判断精牌
		bool IsKingCardData(BYTE cbCardData);
		//判断精牌
		bool IsKingCardIndex(BYTE cbCardIndex);

		//有效判断
		bool IsValidCard(BYTE cbCardData);
		//扑克数目
		BYTE GetCardCount(BYTE cbCardIndex[MAX_INDEX]);
		//组合扑克
		BYTE GetWeaveCard(BYTE cbWeaveKind, BYTE cbCenterCard, BYTE cbCardBuffer[4]);

		//等级函数
	public:
		//动作等级
		BYTE GetUserActionRank(DWORD wUserAction);
		//取出动作数组
		void GetAllAction(DWORD actionMask,vector<BYTE>& actions);
		//取出一个动作
		BYTE GetOneAction(DWORD actionMask);
		BYTE  GetOneAIAction(DWORD actionMask, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, BYTE cbCurrentCard,uint16 curChairID, BYTE cbListenStatus[GAME_PLAYER]);

		//动作判断
	public:
		//吃牌判断
		DWORD EstimateEatCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbCurrentCard);
		//碰牌判断
		DWORD EstimatePengCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbCurrentCard);
		//杠牌判断
		DWORD EstimateGangCard(BYTE cbCardIndex[MAX_INDEX], BYTE cbCurrentCard);

		//动作判断
	public:
		//杠牌分析
		DWORD AnalyseGangCard(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, stGangCardResult & GangCardResult);
		//吃胡分析
		DWORD AnalyseChiHuCard(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, BYTE cbCurrentCard, DWORD wChiHuRight, stChiHuResult & ChiHuResult, bool bZimo=false);
		DWORD AnalyseTingCard19(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<tagAnalyseTingNotifyHu > & vecNotifyHuCard);
		//听牌分析(能和)
		DWORD AnalyseTingCard18(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount);
		//听牌分析(返回能出的牌、胡的牌、胡分数、胡数量)
		DWORD AnalyseTingCard17(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, map<BYTE, vector<tagAnalyseTingNotifyHu>>& mpTingNotifyHu);
		//听牌分析(胡的牌、胡分数、胡数量)
		DWORD AnalyseTingCard16(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<tagAnalyseTingNotifyHu> & vecNotifyHuCard);
		//听牌分析(返回能出的牌、胡的牌、胡分数、胡数量)
		DWORD AnalyseTingCard15(uint8 chairID, BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, map<BYTE, vector<tagAnalyseTingNotifyHu>>& mpTingNotifyHu);
		//听牌分析(返回能出的牌)
		DWORD AnalyseTingCard14(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[],BYTE cbWeaveCount,vector<BYTE>& outCards);
		//听牌分析(能和)
		DWORD AnalyseTingCard13(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount);
		//听牌分析(返回能出的牌和胡的牌)
		DWORD AnalyseTingCard12(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<BYTE>& outCards, vector<BYTE>& huCards);
		//听牌分析(返回能出的牌和胡的牌)
		DWORD AnalyseTingCard11(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, vector<BYTE>& huCards);

		//补杠听牌
		DWORD AnalyseBuGangTing(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[],BYTE cbWeaveCount,BYTE cbCard);

	public:
		uint32 GetAnalyseChiHuScore(stChiHuResult & result);
		uint32 GetAnalyseChiHuCardCount(uint8 chairID, BYTE cbCard);

		uint32 GetAnalyseChiHuCardPoolCount(uint8 chairID, BYTE cbCard);

		//转换函数
	public:
		//扑克转换
		BYTE SwitchToCardData(BYTE cbCardIndex);
		//扑克转换
		BYTE SwitchToCardIndex(BYTE cbCardData);
		//扑克转换
		BYTE SwitchToCardData(BYTE cbCardIndex[MAX_INDEX], BYTE cbCardData[MAX_COUNT],BYTE bMaxCount);
		//扑克转换
		BYTE SwitchToCardIndex(BYTE cbCardData[], BYTE cbCardCount, BYTE cbCardIndex[MAX_INDEX]);
		//扑克转换
		BYTE SwitchToCardIndex(vector<BYTE> vecCardData,BYTE cbCardIndex[MAX_INDEX]);

	public:
		// 是不是2、5、8将牌
		bool IsLeaderCard258(BYTE cardValue);
		// 是否需要258将
		bool IsNeed258Leader(stActionOper& oper);
		// 是否需要替换赖子
		bool IsNeedKingComb(BYTE cbCardIndex[MAX_INDEX],BYTE cbIndex,BYTE pos);

		//内部函数
	private:
		//分析扑克
		bool AnalyseCard(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbItemCount,vector<stAnalyseItem> & AnalyseItemArray,uint32 & kind);
		//分析扑克
		bool AnalyseCardNoKing(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbItemCount,vector<stAnalyseItem> & AnalyseItemArray);

		//七小对牌
		uint32 IsQiXiaoDui(const BYTE cbCardIndex[MAX_INDEX], const stWeaveItem WeaveItem[], const BYTE cbWeaveCount);
		//全分离
		bool IsNeatAlone(BYTE cbCardIndex[MAX_INDEX]);
		// 清一色
		bool IsQingYiSe(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo);
		// 全求人
		bool IsQuanQiuRen(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo);

		// 门清(没有吃碰明杠)
		bool IsMenQing(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo);

		// 小胡判断
	public:
		void CheckMinHu(BYTE cbCardIndex[MAX_INDEX],stActionOper& oper);
		// 国标麻将算番
		uint32 CalcGuoBiaoFan(stChiHuResult& result);

		// 国标麻将移除叠加番型
		void RemoveGuoBiaoFan(stChiHuResult& result);
		// 国标麻将牌型计算
		//void CalcGuoBiaoHuCardType(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,const BYTE cbCurrentCard,bool bZimo,stAnalyseItem* pAnalyseItem,stActionOper& oper);
		void CalcHuCardType(BYTE cbCardIndex[MAX_INDEX], stWeaveItem WeaveItem[], BYTE cbWeaveCount, const BYTE cbCurrentCard, bool bZimo, stAnalyseItem* pAnalyseItem, stActionOper& oper);

		bool CheckAddHuCardType(DWORD dwChiHuRight, stActionOper & ChiHuKind);

		void BubbleSort(BYTE cbArr[], int iCount, bool bBig = false);

		// 判断是否存在相应刻子
		bool IsExistKeZi(BYTE card,stAnalyseItem* pAnalyseItem);
		bool IsExistGang(BYTE card,stAnalyseItem* pAnalyseItem);
		bool IsWanZi(BYTE card);
		bool IsSuoZi(BYTE card);
		bool IsTongZi(BYTE card);
		bool IsFengZi(BYTE card);
		bool IsJianZi(BYTE card);
		bool IsZiPai(BYTE card);
		BYTE GetEatCenterCard(DWORD action,BYTE centerCard);
		bool  m_bIsAnalyseTingCard15;
		// 特殊胡牌
	public:
		// 起手4红中
		bool IsHongZhongHu(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount,BYTE cbCurrentCard);
		// 将将胡
		bool IsJiangJiangHu(BYTE cbCardIndex[MAX_INDEX],stWeaveItem WeaveItem[], BYTE cbWeaveCount);


	};
};

#endif

