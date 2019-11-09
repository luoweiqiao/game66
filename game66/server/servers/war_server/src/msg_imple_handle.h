
#ifndef SERVER_MSG_IMPLE_HANDLE_H
#define SERVER_MSG_IMPLE_HANDLE_H

#include "msg_lobby_handle.h"

class CHandleImpleMsg : public CHandleLobbyMsg
{
public:
    // 游戏内消息
    virtual int  handle_msg_gameing_oper(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);


};




#endif //SERVER_MSG_LAND_HANDLE_H
