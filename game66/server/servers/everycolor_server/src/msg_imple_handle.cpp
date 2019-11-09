
#include "msg_imple_handle.h"
#include "game_room_mgr.h"
#include "stdafx.h"
#include "pb/msg_define.pb.h"

// 游戏内消息
int  CHandleImpleMsg::handle_msg_gameing_oper(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
	
	uint32 uid = 0;
	CGameTable* pGameTable = NULL;
	if (pGamePlayer != NULL)
	{
		uid = pGamePlayer->GetUID();
		pGameTable = pGamePlayer->GetTable();
	}
	//LOG_DEBUG("game server msg - uid:%d,uin:%d,cmd:%d,pGameTable:%p", uid,head->uin,head->cmd, pGameTable);
    if(pGamePlayer == NULL) {
        return 0;
    }
	
    if(pGameTable != NULL)
    {
        pGameTable->OnGameMessage(pGamePlayer,head->cmd,pkt_buf,buf_len);
    }
    return 0;
}