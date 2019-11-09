#ifndef CENTER_LOG_MGR_H__
#define CENTER_LOG_MGR_H__

#include <string>
#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include "db_struct_define.h"
#include "dbmysql_mgr.h"
#include <vector>
#include "game_define.h"
#include "helper/fileStream.h"

using namespace std;
using namespace svrlib;

enum emLOG_FILE_TYPE
{
  emCENTERLOG_FILE_TYPE_NORMAL = 0, // 普通日志
  emCENTERLOG_FILE_TYPE_GAMELOG,    // 牌局文件
  emCENTERLOG_FILE_TYPE_ACTION,     // 动作日志    
  
  emCENTERLOG_FILE_TYPE_MAX,
};
enum emUSER_ACTION_TYPE
{
    emUSER_ACTION_TYPE_LOGIN     = 1,  // 登陆
    emUSER_ACTION_TYPE_LOGINOUT  = 2,  // 离线
};

enum emACCTRAN_OPER_TYPE
{
    emACCTRAN_OPER_TYPE_PAY         = 1,  // 充值
    emACCTRAN_OPER_TYPE_GIVE        = 2,  // 赠送
    emACCTRAN_OPER_TYPE_GAME        = 3,  // 游戏输赢
    emACCTRAN_OPER_TYPE_TASK        = 4,  // 任务奖励
    emACCTRAN_OPER_TYPE_FEE         = 5,  // 台费
    emACCTRAN_OPER_TYPE_BUY         = 6,  // 购买
    emACCTRAN_OPER_TYPE_EXCHANGE    = 7,  // 兑换
    emACCTRAN_OPER_TYPE_LOGIN       = 8,  // 登陆奖励
    emACCTRAN_OPER_TYPE_BANKRUPT    = 9,  // 破产补助
    emACCTRAN_OPER_TYPE_MAIL        = 10, // 邮件领取
    emACCTRAN_OPER_TYPE_SAFEBOX     = 11, // 保险箱操作
    emACCTRAN_OPER_TYPE_PROOM       = 12, // 开房费
    emACCTRAN_OPER_TYPE_ROBOTPAY    = 13, // 机器人充值
    emACCTRAN_OPER_TYPE_ROBOTSTORE  = 14, // 机器人存入
    emACCTRAN_OPER_TYPE_BACKSITE    = 15, // PHP后台赠送
    emACCTRAN_OPER_TYPE_REBATE      = 16, // VIP赠送返利
    emACCTRAN_OPER_TYPE_SPEAK       = 17, // 喇叭消耗
    emACCTRAN_OPER_TYPE_LOCK        = 18, // 冻结解冻
    emACCTRAN_OPER_TYPE_JUMPQUEUE   = 19, // 插队费用
    emACCTRAN_OPER_TYPE_BIGROLL     = 20, // 大转盘
    emACCTRAN_OPER_TYPE_BUYIN       = 21, // buyin
	emACCTRAN_OPER_TYPE_TIXIAN		= 22, // 
    emACCTRAN_OPER_TYPE_SCORETOCOIN	= 23, // 积分兑换
    emACCTRAN_OPER_TYPE_CHUJIN      = 24, // 
    emACCTRAN_OPER_TYPE_HUODONG     = 25, //活动奖励
    emACCTRAN_OPER_TYPE_CHUJINFAIL  = 26, //提款失败
	emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE = 41, //时时彩游戏内分数更新
	emACCTRAN_OPER_TYPE_DICE_WIN_JACKPOT_SCORE = 43, //骰宝中奖池加分

};
// 牌局日志
struct stBlingUser
{
    uint32 uid;
	uint8  playerType;
	uint8  welfare;
    int64  oldValue;
    int64  newValue;
    int64  safeCoin;
    int64  win;
    uint8  land;
    int64  fee;           // 台费
    uint16 chairid;
	int64  totalwinc;
	int64  stockscore;
	int64  gamecount;
	int64 lJackpotScore;
	uint32		 fish_novice_times;			//新手福利统计次数---捕鱼专用
	uint32		 fish_novice_win_socre;		//新手福利统计金额---捕鱼专用
    stBlingUser(){
        memset(this,0,sizeof(stBlingUser));
    }
};
struct stGameBlingLog
{
    uint32       roomID;              // 房间类型
    uint32       tableID;             // 桌子ID
    uint16       gameType;            // 游戏类型
	int          welfare;
	int          welctrl;
    uint16       roomType;            // 房间类型
    uint8        consume;             // 消费类型
    uint8        deal;                // 发牌类型
    int64        baseScore;           // 底分
    uint32       startTime;           // 开始时间
    uint32       endTime;             // 结束时间
    vector<stBlingUser> users;        // 用户数据
    stringstream operLog;             // 操作日志
    string       chessid;             // 牌局ID
	
    stGameBlingLog(){
        Reset();
    }
    void Reset(){
        roomID      = 0;
        tableID     = 0;            // 桌子ID
        gameType    = 0;            // 游戏类型
        roomType    = 0;            // 房间类型
        consume     = 0;            // 消费类型
        deal        = 0;            // 发牌类型
        baseScore   = 0;            // 底分
        startTime   = 0;            // 开始时间
        endTime     = 0;            // 结束时间
        users.clear();              // 用户数据
        operLog.str("");
        chessid     ="";		
    }
};

// 广播
struct stGameBroadcastLog
{
	uint32       uid;                 //用户ID
	uint16       actType;             //操作类型
	uint32       gameType;            //游戏类型
	int64        score;               //赢得金币数

	stGameBroadcastLog() {
		Reset();
	}
	void Reset() {
		uid = 0;               //用户ID
		actType = 0;           //操作类型
		gameType = 0;          //游戏类型
		score = 0;            //赢得金币数
	}
};

/*************************************************************/
class CCenterLogMgr : public ITimerSink, public AutoDeleteSingleton<CCenterLogMgr>
{
private:

public:
    CCenterLogMgr();
    virtual ~CCenterLogMgr();

public:
    bool    Init(uint16 svrID);
    void    ShutDown();
    void    OnTimer(uint8 eventID);

    // 在线玩家金流日志
    void    AccountTransction(uint32 uid,uint16 atype,uint16 ptype,uint32 sptype,int64 amount,int64 oldv,int64 newv,const string& chessid);
    // 离线玩家金流日志
    void    OfflineAccountTransction(uint32 uid,uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid,bool haveold, stAccountInfo & data);


	//广播日志
	void   WriteGameBroadcastLog(stGameBroadcastLog& log);
    // 牌局日志
    void    WriteGameBlingLog(stGameBlingLog& log);
    // 动作日志
    void    UserActionLog(uint32 uid,uint16 act,int64 value = 1);
	// 错误的SQL语句
	void	WriteErrorMysqlLog(const string& sql);

protected:
    void	CheckTransFile();
    void	WriteLog(uint32 logFileType, const char *logdata);

private:
    vector<CFileStream>   m_vecFiles;
    vector<string>        m_vecNames;
    string                m_sdir;
    uint16                m_svrID;
    CTimer*               m_pTimer;
    CFileStream           m_mysqlFile;  
    
};




#endif //
