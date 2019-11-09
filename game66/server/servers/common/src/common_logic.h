
#ifndef COMMON_LOGIC_H__
#define COMMON_LOGIC_H__

#include "svrlib.h"
#include <iostream>
#include <vector>
#include <algorithm>

class CPlayer;
class CPlayerBase;

using namespace std;
using namespace svrlib;


class CCommonLogic
{
public:
    // 检测时间是否在区间
    static bool     IsJoinTime(string startTime,string endTime);
    // 打印出牌字符串
    static void     LogCardString(uint8 cardData[],uint8 cardCount);
    static void     LogIntString(int64 value[],int32 count);
    static int64    SumIntArray(int64 value[],int32 count);

    // 获得校验码
    static string   VerifyPasswd(uint32 uid,string device);

    static uint32   GetDataTableNum(uint32 uid);

	//有新手福利功能的非百人游戏
	static bool     NotIsBaiRenNewPlayerNoviceWelfare(uint16 gameType);
	//有新手福利功能的百人游戏
	static bool     IsBaiRenNewPlayerNoviceWelfare(uint16 gameType);
	//有新手福利功能的游戏
	static bool     IsNewPlayerNoviceWelfare(uint16 gameType);

	//有自动杀分功能的非百人游戏
	static bool     NotIsBaiRenAutoKillScore(uint16 gameType);
	//有自动杀分功能的百人游戏
	static bool     IsBaiRenAutoKillScore(uint16 gameType);
	
	// 根据房间配置机器人在线数量的百人游戏
	static bool     IsBaiRenRobotGame(uint16 gameType);
	// 百人游戏
    static bool     IsBaiRenGame(uint16 gameType);
	// 非百人游戏 （过滤站起坐下功能）
	static bool     NotIsBaiRenGame(uint16 gameType);

	// 统计前20局的百人游戏
	static bool     IsBaiRenCount(uint16 gameType);
    // 是否开放这个游戏
    static bool     IsOpenGame(uint16 gameType);

    // 判断是否重置信息
    static bool		IsNeedReset(uint32 lastTime,uint32 curTime);
    // 原子修改离线玩家数据
    static bool    AtomChangeOfflineAccData(uint32 uid,uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid="");

	//判断是否当前游戏是否有幸运值功能
	static bool     IsLuckyFuncitonGame(uint16 gameType);
    
};





































#endif // COMMON_LOGIC_H__


