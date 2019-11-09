#ifndef GAME_FRUIT_LOGIC_HEAD_FILE
#define GAME_FRUIT_LOGIC_HEAD_FILE
#include "svrlib.h"

using namespace std;
#define PRO_DENO_100     100	
#define MAXROW                       3            //最大行数	
#define MAXCOLUMN                    5            //数目排序
#define MAXWEIGHT                    11           //权重个数


#define	PIC_SEVEN                    11          //特殊图片		7
#define	PIC_WILD                     9           //万能图片		百搭
#define	PIC_BONUS                    10           //特殊图片	免费转动

#define	MAX_LINE                     9             //最大线数
//#define	MAX_WHEELS               100000        //最大滚轴图集的个数
#define	MAX_WHEELS                   312			//最大滚轴图集的个数
#define MAX_RATIO                    100000000      //最大精度

//线的编号
typedef struct
{
	uint32				lineID; ////线id
	uint32				pos[MAXCOLUMN]; //线对应的点
}LinePosConfig;
typedef vector<LinePosConfig> ServerLinePosVecType;

//图片对应的赔率
typedef struct
{
	uint32				picId;		           //图片id
	uint32				multiple[MAXCOLUMN];	//赔率
}PicRateConfig;
typedef vector<PicRateConfig> PicRateConfigVecType;

//连线结果记录
typedef struct
{
	uint32 	lineNo;  //线号
	uint32	picID;	//图片ID
	uint32 	linkNum; //连接数量
}picLinkinfo;
typedef vector<picLinkinfo> picLinkVecType;

//水果机图集合的编号
typedef struct
{
	uint32			id;          //序号
	uint32			pic[MAXROW][MAXCOLUMN];     //对应点 矩阵
	uint32          LineMultiple[MAX_LINE];     //1-9线总赔率
	uint32          LineBonus[MAX_LINE];        //1-9线免费次数
	uint32          SysRatio[MAX_LINE][5];      //9线系统返奖1-5调节
	uint32          UserRatio[MAX_LINE][2];     //9线个人返奖1-2调节
	uint32          TotalSysRatio[MAX_LINE][5];      //9线系统返奖1-5调节和
	uint32          TotalUserRatio[MAX_LINE][2];     //9线个人返奖1-2调节和
}WheelInfo;
typedef vector<WheelInfo> ServerWheelInfoType;

//水果机图集合的编号
typedef struct
{
	uint32 linenum;//线号
	uint32 index;  //索引号
	uint32 compRate;   //查找的返奖调节 
}compareDef;

//点的序号
//1   4   7   10   13
//2   5   8   11   14
//3   6   9   12   15

//[2, 5, 8, 11, 14],
//[1, 4, 7, 10, 13],
//[3, 6, 9, 12, 15],
//[1, 5, 9, 11, 13],
//[3, 5, 7, 11, 15],
//[1, 4, 8, 12, 15],
//[3, 6, 8, 10, 13],
//[2, 6, 8, 10, 14],
//[2, 4, 8, 12, 14]


//[20, 43, 43, 43, 72, 38, 28, 60, 20, 10, 0],
//[20, 43, 43, 43, 72, 38, 28, 50, 50, 20, 0], 
//[80, 43, 43, 43, 72, 38, 28, 40, 50, 20, 0], 
//[80, 43, 88, 58, 72, 38, 28, 25, 60, 12, 0], 
//[80, 43, 88, 58, 72, 38, 28, 25, 10, 13, 0]


namespace game_slot
{
	bool compareBySysKey(const WheelInfo & info, const compareDef & defRate);
	bool compareByUseKey(const WheelInfo & info, const compareDef & defRate);
	//游戏逻辑
	class CSlotLogic
	{
		//函数定义
	public:
		//构造函数
		CSlotLogic();
		//析构函数
		virtual ~CSlotLogic();
		void Init();
		void MakeWheels();
		void MakeNoRateWheels();
		//变量定义
	public:
		//计算相连的个数
		uint64 GetLineMultiple(uint32 GamePic[MAXCOLUMN], picLinkinfo &picLink);
		uint32 m_GamePic[MAXROW][MAXCOLUMN];//矩阵3*5

		bool GetRandPicArray(uint32 GamePic[][MAXCOLUMN]);

		uint32 GetMultipleRandIndex(uint32 num, uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple);

		bool GetMultiplePicArray(uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple,uint32 GamePic[][MAXCOLUMN]);

		//获取系统返奖调节图片矩阵
		bool GetSysPicArray(uint32 randRate, uint32 linenum, uint32 ReturnIndex, uint32 GamePic[]);
		//获取个人返奖调节图片矩阵
		bool GetUserPicArray(uint32 randRate, uint32 linenum, uint32 ReturnIndex, uint32 GamePic[]);
		//根据权重算法,获取拿到第几大的牌型
		uint32 GetWeightIndex(uint32 num);
		uint32 GetFreeSpinWeightIndex(uint32 num);
		uint32 GetMultipleSpinWeightIndex(uint32 num, uint32 nline, int64 lMinWinMultiple, int64 lMaxWinMultiple);

		//测试根据权重算法,获取拿到第几大的牌型
		uint32 GetNoRateWeightIndex(uint32 posone, uint32 postwo, uint32 posthree);
		void   testMakeWheels();
		void	GetJackpotScoreByLine(uint32 line, uint32 mGamePic[][MAXCOLUMN]);

		void GetJackpotScoreOne(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreTwo(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreThree(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreFour(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreFive(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreSix(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreSeven(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreEight(uint32 mGamePic[][MAXCOLUMN]);
		void GetJackpotScoreNine(uint32 mGamePic[][MAXCOLUMN]);


		uint32 GetBananaWeightIndex(uint32 line);
		//初始化函数
		void SetSlotFeeRate(int64 nRate) { m_FeeRate = nRate; }
		void SetSlotJackpotRate(int64 nRate) { m_JackpotRate = nRate; }
		void SetSlotUserBet(uint64 bet) { m_UserBet = bet; }
		void SetWeightPicCfg(uint32 weightPicCfg[]) { memcpy(m_WeightPicCfg, weightPicCfg, 220); }
		void SetBonusFreeTimes(uint32 BonusFreeTimes[]) { memcpy(m_BonusFreeTimes, BonusFreeTimes,  MAXCOLUMN*4); }
		void SetReturnUserRate(int32 ReturnUserRate[]) { memcpy(m_ReturnUserRate, ReturnUserRate, 2*4); }
		void SetReturnSysRate(int32 ReturnSysRate[])   { memcpy(m_ReturnSysRate, ReturnSysRate, 5*4); }
		void SetServerLinePos(ServerLinePosVecType pos) { m_LogicLinePos = pos; }
		void SetServerLinePos(PicRateConfigVecType cfg) { m_LogicPicRate = cfg; }
	protected:

    // 读取配置的变量
	protected:
		uint32                         m_WeightPicCfg[MAXCOLUMN][MAXWEIGHT];  // 权重配置表
		ServerWheelInfoType            m_WheelInfo;                           //水果机所有图库集合
		ServerWheelInfoType            m_FindWheelInfo;                       //水果查找图库集合
		ServerLinePosVecType           m_LogicLinePos;                       //线对应的点
		PicRateConfigVecType           m_LogicPicRate;                       //图片对应的赔率
		uint64                         m_UserBet;                            //单线的押注
		uint32                         m_BonusFreeTimes[MAXCOLUMN];           //bonus免费次数
		int64                         m_FeeRate;                             // 抽水比例
		int64                         m_JackpotRate;                         // 彩金比例
		int32                         m_ReturnUserRate[2];                   //个人返奖比例
		int32                         m_ReturnSysRate[5];                    // 系统返奖比例
		bool                           m_IsNewWheel;                       //是否有更新
		ServerWheelInfoType            m_NoRateWheel;                           //没有赔率的图库集
	};
}
#endif