

#include "msg_imple_handle.h"
#include "game_room_mgr.h"
#include "stdafx.h"
#include "pb/msg_define.pb.h"

// 游戏内消息
int  CHandleImpleMsg::handle_msg_gameing_oper(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    //LOG_DEBUG("游戏内部消息:uid:%d--msg:%d",head->uin,head->cmd);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL) {
        return 0;
    }
    CGameTable* pGameTable = pGamePlayer->GetTable();
    if(pGameTable != NULL)
    {
        pGameTable->OnGameMessage(pGamePlayer,head->cmd,pkt_buf,buf_len);
    }
    return 0;
}