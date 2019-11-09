
#ifndef __DBMYSQL_MGR_H__
#define __DBMYSQL_MGR_H__

#include <string>
#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include "dbmysql/dbmysql.h"
#include <vector>
#include "db_operator.h"
#include "db_struct_define.h"
#include <queue>
#include "dbmysql/db_task.h"
#include "game_define.h"
#include "config/config.h"

using namespace std;
using namespace svrlib;

class AsyncDBCallBack
{
public:    
    virtual bool OnProcessDBEvent(CDBEventRep* pRep) = 0;    
};
class CDBTaskImple : public CDBTask
{
public:
    virtual void writeLog(string logStr);    
};
class CDBMysqlMgr : public ITimerSink,public AutoDeleteSingleton<CDBMysqlMgr>
{
public:
	CDBMysqlMgr();
	~CDBMysqlMgr();

    virtual void OnTimer(uint8 eventID);

	bool	Init(stDBConf szDBConf[]);
	void	ShutDown();
    void    SetAsyncDBCallBack(AsyncDBCallBack* pCallBack);
    
    void    ProcessDBEvent();
    // 加载玩家数据
    void    AsyncLoadPlayerData(uint32 uid);
    void    AsyncLoadAccountData(uint32 uid);
    void    AsyncLoadMissionData(uint32 uid);
    // 加载游戏数据
    void    AsyncLoadGameData(uint32 uid,uint16 gameType);
	//void    AsyncLoadPayData(uint32 uid);

    // 加载登陆机器人
    bool    AsyncLoadRobotLogin(uint16 gameType,uint8 level, uint8 day,uint32 robotNum,uint32 batchID, uint32 loginType, uint32 leveltype);
	void    AsyncLoadTimeIntervalRobotLogin(uint16 gameType, uint32 roomid, uint8 level, uint8 day, uint32 robotNum, uint32 batchID, uint32 loginType);

	
    // 添加异步SQL语句
	void	AddAsyncSql(uint8 dbType,string strSql);
// 数据库操作接口    
public:
    // 更新服务器配置信息
    void    UpdateServerInfo();
    // 上报服务器在线人数
    void    ReportOnlines();

    // 更新玩家在线服务器房间
    void    UpdatePlayerOnlineInfo(uint32 uid,uint32 svrid,uint32 roomid,uint8 playerType,int64 coin = 0,int64 safecoin = 0,int64 score = 0,string strCity="");
	void    UpdatePlayerOnlineInfoEx(uint32 uid, uint32 svrid, uint32 roomid, uint8 playerType, int64 coin = 0, int64 safecoin = 0, int64 score = 0, string strCity = "");
    void    ClearPlayerOnlineInfo(uint32 svrid);
    
    // 更新连续登陆奖励
    void    UpdatePlayerLoginInfo(uint32 uid,uint32 offlinetime, uint32 clogin,uint32 weeklogin,uint32 reward,uint32 bankrupt,uint32 dgameCount);
    void    UpdatePlayerLoginTime(uint32 uid,uint32 logintime,string loginip);
    void    AddPlayerLoginTimeInfo(uint32 uid,uint32 days,uint32 playTime);
	void    UpdatePlayerClogin();

    // 设置玩家保险箱密码
    void    UpdatePlayerSafePasswd(uint32 uid,string passwd);
    // 修改玩家账号数值（增量修改）
    bool    ChangeAccountValue(uint32 uid,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin);
    bool    AtomChangeAccountValue(uint32 uid,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin);
    void    ChangeFeeValue(uint32 uid,int64 feewin,int64 feelose);
    
    // 增加玩家破产次数
    void    AddBankruptValue(uint32 uid);
    // 增加机器人登陆次数
    void    AddRobotLoginCount(uint32 uid);
    void    SetRobotLoginState(uint32 uid,uint8 state);
    // 清空机器人登陆状态
    void    ClearRobotLoginState();

    // 保险箱赠送操作
    void    GiveSafeBox(uint32 suid,uint32 ruid,int64 coin,int64 tax,string strIp);
    
    // 插入游戏数据记录
    void    InsertGameValue(uint16 gameType,uint32 uid);
    // 修改游戏数据记录
    void    ChangeGameValue(uint16 gameType,uint32 uid, bool bIsRobot, bool isCoin,int32 win,int32 lose,int64 winscore, int64 lExWinScore,int64 maxscore,int rwelfare,int welcount);
    // 更新最大牌型
    void    UpdateGameMaxCard(uint16 gameType,uint32 uid,bool isCoin,uint8 cardData[],uint8 cardCount);
    // 重置每日盈利
    void    ResetGameDaywin(uint16 gameType,uint32 uid);
    
	void    ResetGameWeekWin(uint16 gameType);

	void    ResetGameWeekWin(uint16 gameType, uint32 uid);

    // 修改斗地主数值(增量修改)
    void    ChangeLandValue(uint32 uid,bool isCoin,int32 win,int32 lose,int32 land,int32 spring,int64 maxScore);
    
    // 保存用户任务信息
    void    SaveUserMission(uint32 uid,map<uint32,stUserMission>& missions);
    // 更新私人房收益
    void    ChangePrivateTableIncome(uint32 tableID,int64 hostIncome,int64 sysIncome);
    // 更新私人房过期时间
    void    UpdatePrivateTableDuetime(uint32 tableID,uint32 duetime);
    // 插入充值订单记录
    void    InsertPayLog(uint32 uid,int64 rmb,int64 diamond);
    // 发送邮件给玩家
    void    SendMail(uint32 sendID,uint32 recvID,string title,string content,string nickname);

	//异步保存夺宝数据
	void    AsyncSaveSnatchCoinGameData(uint32 uid, uint32 type, string card);

	void    AsyncSaveUserSnatchGameData(uint32 uid, uint32 snatch_type, uint32 player_type, string periods, string card);

	//清除夺宝数据
	void	ClearSnatchCoinGameData();

	void	ClearUserSnatchGameData(uint32 snatch_type);

	void    AsyncSaveUserBankruptScore(uint32 uid, uint16 gameType, uint16 roomID, int64 oldValue, int64 newValue, int64 enter_min, uint64 utime);

	// 修改用户库存数值（增量修改）
	void    ChangeStockScore(uint16 gametype, uint32 uid, int64 score);
	void	UpdateGameRoomParam(string param, uint16 gametype, uint16 roomid);

	void    AsyncInsertBairenCount(struct tagBairenCount & BairenCount);
	void    AsyncRemoveBairenCount(struct tagBairenCount & BairenCount);
	void  ClearBairenCount();


	void    SaveControlPlayerData(struct tagControlPalyer & ControlPalyer);
	void    SaveControlPlayerData(struct tagControlMultiPalyer & ControlPalyer);

    // 添加自动杀分数据记录
    //void    AddAutoKillData(uint32 uid,uint32 gid,uint32 rid,int64 newSrcoe,int64 oldSrcoe);
    // 更新自动杀分玩家数据
    //void    SaveAutoKillPlayerData(uint32 uid,int64 coin);

    // 插入活跃福利的玩家充值与提现数据
    void    InsertActiveWelfare(uint32 uid, int64 recharge_actwle, int64 converts_actwle);

    // 记录玩家活跃福利的数据
    void    InsertActiveWelfareLog(uint32 uid, uint8 game_id, int64 base_loss, string loss_range, uint64 welfare, uint8 status, int64 current_loss, int64 win_coin, uint64 sys_time);
    
	// 记录新注册玩家福利的数据
	void    InsertNewRegisterWelfareLog(uint32 uid, uint8 game_id, uint32 curr_must_win, uint32 curr_total_win, uint64 curr_win_coin, uint64 total_win_coin, uint32 status, uint64 sys_time);

	// 更新库存信息 add by har
    // addStockScore : 实时库存增加值
    // addJackpot : 实时奖池增加值
	void UpdateStock(uint16 gameType, uint16 roomId, int64 addStockScore, int64 addJackpot);

	// 修改捕鱼游戏数据记录
	void    ChangeGameValueForFish(uint32 uid, bool isCoin, int64 winscore, int welcount);

	// 同步数据库操作
public:
    CDBOperator& GetSyncDBOper(uint8 dbIndex){ return m_syncDBOper[dbIndex]; }
protected:
    void    ZeroSqlBuff(){memset(m_szSql,0,sizeof(m_szSql));}
    // 调用通用sql
    void    SendCommonLog(uint8 dbType);
private:
	// 启动日志异步线程
	bool	StartAsyncDB();
	// 连接配置数据库
	bool	ConnectSyncDB();


private:
    bool    OnProcessDBEvent(CDBEventRep* pRep);
    

private:
    // 同步数据库操作
	CDBOperator		m_syncDBOper[DB_INDEX_TYPE_MAX];	// 同步数据库
    // 异步数据库操作	
	CDBTaskImple* 	m_pAsyncTask[DB_INDEX_TYPE_MAX];	// 异步数据库线程
	
	stDBConf	    m_DBConf[DB_INDEX_TYPE_MAX];
    AsyncDBCallBack* m_pAsyncDBCallBack;
    
    uint16          m_svrID;
    char            m_szSql[4096];
    CTimer*         m_pReportTimer;

};

#endif // __DBMYSQL_MGR_H__


