
#ifndef LAND_LOGIC_HEAD_FILE
#define LAND_LOGIC_HEAD_FILE


#include "svrlib.h"
#include "poker/poker_logic.h"

using namespace std;
using namespace svrlib;

namespace game_land
{

//数目定义
static const int  GAME_LAND_PLAYER  = 3;                                     // 斗地主游戏人数
static const int  MAX_LAND_COUNT    = 20;                                    // 最大数目

//逻辑数目
static const int NORMAL_COUNT 		= 17;                                    //常规数目
static const int DISPATCH_COUNT 	= 51;                                    //派发数目
static const int GOOD_CARD_COUTN	= 38;									 //好牌数目

static const int MAX_TYPE_COUNT     = 254;

#define MAX_SIG_CARD_COUNT			16

//逻辑类型
enum emLAND_CARD_TYPE {
	CT_ERROR = 0,                                 //错误类型
	CT_SINGLE,                                    //单牌类型
	CT_DOUBLE,                                    //对牌类型
	CT_THREE,                                     //三条类型
	CT_SINGLE_LINE,                               //单连类型
	CT_DOUBLE_LINE,                               //对连类型
	CT_THREE_LINE,                                //三连类型
	CT_THREE_TAKE_ONE,                            //三带一单
	CT_THREE_TAKE_TWO,                            //三带一对
	CT_FOUR_TAKE_ONE,                             //四带两单
	CT_FOUR_TAKE_TWO,                             //四带两对
	CT_BOMB_CARD,                                 //炸弹类型
	CT_MISSILE_CARD,                              //火箭类型
};

//排序类型
#define ST_ORDER                    1                                    //大小排序
#define ST_COUNT                    2                                    //数目排序
#define ST_CUSTOM                   3                                    //自定排序

//////////////////////////////////////////////////////////////////////////////////

//分析结构
struct tagAnalyseResult {
	BYTE cbBlockCount[4];                    //扑克数目
	BYTE cbCardData[4][MAX_LAND_COUNT];        //扑克数据
};

//出牌结果
	struct tagOutCardResult {
		BYTE cbCardCount;                        //扑克数目
		BYTE cbResultCard[MAX_LAND_COUNT];        //结果扑克
	};

//分布信息
	struct tagDistributing {
		BYTE cbCardCount;                        //扑克数目
		BYTE cbDistributing[15][6];                //分布信息
	};

//搜索结果
	struct tagSearchCardResult {
		BYTE cbSearchCount;                                    //结果数目
		BYTE cbCardCount[MAX_LAND_COUNT];                      //扑克数目
		BYTE cbResultCard[MAX_LAND_COUNT][MAX_LAND_COUNT];     //结果扑克
	};

    struct tagOutCardTypeResult 
    {
    	BYTE							cbCardType;							//扑克类型
    	BYTE							cbCardTypeCount;					//牌型数目
    	BYTE							cbEachHandCardCount[MAX_TYPE_COUNT];//每手个数
    	BYTE							cbCardData[MAX_TYPE_COUNT][MAX_LAND_COUNT];//扑克数据
    };

//////////////////////////////////////////////////////////////////////////////////

//游戏逻辑类
	class CLandLogic : public CPokerLogic
	{
		//变量定义
	public:
		static const BYTE m_cbCardData[FULL_POKER_COUNT];            //扑克数据
	    //AI变量
    public:
    	//static const BYTE				m_cbGoodcardData[GOOD_CARD_COUTN];	                                    //好牌数据
    	BYTE							m_cbAllCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];                      //所有扑克
    	BYTE							m_cbLandScoreCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];	            //叫牌扑克
    	BYTE							m_cbUserCardCount[GAME_LAND_PLAYER];		                            //扑克数目
    	WORD							m_wBankerUser;						                                    //地主玩家

		//函数定义
	public:
		//构造函数
		CLandLogic();

		//析构函数
		virtual ~CLandLogic();

		//类型函数
	public:
		//获取类型
		BYTE GetCardType(const BYTE cbCardData[], BYTE cbCardCount);
		//控制函数
	public:
		//混乱扑克
		void RandCardList(BYTE cbCardBuffer[], BYTE cbBufferCount);
		void ShuffleCard(BYTE cbCardData[], BYTE cbCardCount);

		//排列扑克
		void SortCardList(BYTE cbCardData[], BYTE cbCardCount, BYTE cbSortType);

		//删除扑克
		bool RemoveCardList(const BYTE cbRemoveCard[], BYTE cbRemoveCount, BYTE cbCardData[], BYTE cbCardCount);

		//逻辑函数
	public:
		//逻辑数值
		BYTE GetCardLogicValue(BYTE cbCardData);

		//对比扑克
		bool CompareCard(const BYTE cbFirstCard[], const BYTE cbNextCard[], BYTE cbFirstCount, BYTE cbNextCount);
		//出牌搜索
		bool SearchOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult);
		//内部函数
	public:
		//分析扑克
		void AnalysebCardData(const BYTE cbCardData[], BYTE cbCardCount, tagAnalyseResult &AnalyseResult);

		//分析分布
		void AnalysebDistributing(const BYTE cbCardData[], BYTE cbCardCount, tagDistributing &Distributing);

    	//AI函数
    public:
    	//设置扑克
    	void SetUserCard(WORD wChairID, BYTE cbCardData[], BYTE cbCardCount) ;
    	//设置底牌
    	void SetBackCard(WORD wChairID, BYTE cbBackCardData[], BYTE cbCardCount) ;
    	//设置庄家
    	void SetBanker(WORD wBanker) ;
    	//叫牌扑克
    	void SetLandScoreCardData(WORD wChairID,BYTE cbCardData[], BYTE cbCardCount) ;
    	//删除扑克
    	void RemoveUserCardData(WORD wChairID, BYTE cbRemoveCardData[], BYTE cbRemoveCardCount) ;

    	//辅助函数
    public:
    	//组合算法
    	void Combination(BYTE cbCombineCardData[], BYTE cbResComLen,  BYTE cbResultCardData[254][5], BYTE &cbResCardLen,BYTE cbSrcCardData[] , BYTE cbCombineLen1, BYTE cbSrcLen, const BYTE cbCombineLen2);
    	//排列算法
    	void Permutation(BYTE *list, int m, int n, BYTE result[][4], BYTE &len) ;
    	//分析炸弹
    	void GetAllBomCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbBomCardData[], BYTE &cbBomCardCount);
    	//分析顺子
    	void GetAllLineCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbLineCardData[], BYTE &cbLineCardCount);
    	//分析三条
    	void GetAllThreeCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbThreeCardData[], BYTE &cbThreeCardCount);
    	//分析对子
    	void GetAllDoubleCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbDoubleCardData[], BYTE &cbDoubleCardCount);
    	//分析单牌
    	void GetAllSingleCard(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE cbSingleCardData[], BYTE &cbSingleCardCount);

    	//主要函数
    public:
    	//分析牌型（后出牌调用）
    	void AnalyseOutCardType(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE const cbTurnCardData[], BYTE const cbTurnCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
    	//分析牌牌（先出牌调用）
    	void AnalyseOutCardType(BYTE const cbHandCardData[], BYTE const cbHandCardCount, tagOutCardTypeResult CardTypeResult[12+1]);
    	//单牌个数
    	BYTE AnalyseSinleCardCount(BYTE const cbHandCardData[], BYTE const cbHandCardCount, BYTE const cbWantOutCardData[], BYTE const cbWantOutCardCount, BYTE cbSingleCardData[]=NULL);

    	//出牌函数
    public:
    	//地主出牌（先出牌）
    	void BankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, tagOutCardResult & OutCardResult) ;
    	//地主出牌（后出牌）
    	void BankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wOutCardUser, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    	//地主上家（先出牌）
    	void UpsideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wMeChairID,tagOutCardResult & OutCardResult) ;
    	//地主上家（后出牌）
    	void UpsideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wOutCardUser,  const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    	//地主下家（先出牌）
    	void UndersideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wMeChairID,tagOutCardResult & OutCardResult) ;
    	//地主下家（后出牌）
    	void UndersideOfBankerOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, WORD wOutCardUser, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, tagOutCardResult & OutCardResult) ;
    	//出牌搜索
    	bool SearchOutCard(const BYTE cbHandCardData[], BYTE cbHandCardCount, const BYTE cbTurnCardData[], BYTE cbTurnCardCount, WORD wOutCardUser, WORD wMeChairID, tagOutCardResult & OutCardResult);

    	//叫分函数
    public:
    	//叫分判断
    	BYTE LandScore(WORD wMeChairID, BYTE cbCurrentLandScore) ;

		BYTE GetCardIndex(BYTE cbCardData);





	};
};

#endif

