//
// Created by toney on 16/4/5.
//

#ifndef SERVER_GAME_ROOM_MGR_H
#define SERVER_GAME_ROOM_MGR_H

#include "svrlib.h"
#include "game_room.h"
#include "pb/msg_define.pb.h"

class CGamePlayer;

class CGameRoomMgr : public AutoDeleteSingleton<CGameRoomMgr>
{
public:
    CGameRoomMgr();
    ~CGameRoomMgr();

    bool	Init();
    void	ShutDown();


    CGameRoom* GetRoom(uint32 roomID);
	CGameRoom* GetNewNoviceWelfareRoom(uint32 uid,int bIsNoviceWelfare, int maxjettonrate, uint32 posrmb);
	//CGameRoom* GetAutoKillScoreRoom(uint32 uid, int maxjettonrate);
	CGameRoom* GetEnterRoom(uint32 uid, int maxjettonrate);


    void    SendRoomList2Client(uint32 uid);
    bool    FastJoinRoom(CGamePlayer* pPlayer,uint8 deal,uint8 consume);

    void    GetRoomList(uint8 deal,uint8 consume,vector<CGameRoom*>& rooms);
	bool    GetAllRobotRoom(vector<CGameRoom*> & vecRoomsCoin, vector<CGameRoom*> & vecRoomsScore);
    
	void    OnNewDay();
    // 比较房间函数
    static  bool CompareRoomEnterLimit(CGameRoom* pRoom1,CGameRoom* pRoom2);
    static  bool CompareRoomNums(CGameRoom* pRoom1,CGameRoom* pRoom2);
    static  bool CompareRoomLessNums(CGameRoom* pRoom1,CGameRoom* pRoom2);
    
private:
    typedef 	stl_hash_map<uint32, CGameRoom*>    MAP_ROOMS;
    MAP_ROOMS   m_mpRooms;


};


#endif //SERVER_GAME_ROOM_MGR_H
