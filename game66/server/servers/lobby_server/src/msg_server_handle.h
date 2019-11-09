
#ifndef MSG_SERVER_HANDLE_H__
#define MSG_SERVER_HANDLE_H__

#include "network/protobuf_pkg.h"


using namespace Network;

class CHandleServerMsg : public IProtobufClientMsgRecvSink
{
public: 
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);	

protected:
    // 转发给客户端
    int  route_to_client(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);    
    // 服务器注册 
	int  handle_msg_register_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    // 服务器上报信息
    int  handle_msg_report(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    // 返回大厅
    int  handle_msg_leave_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    // 修改玩家数值
    int  handle_msg_notify_change_account_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 修改玩家数值
	int  handle_msg_update_lobby_change_account_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 修改玩家数值
	int  handle_msg_notify_update_lobby_change_account_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

    // 上报游戏结果
    int  handle_msg_report_game_result(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);   
    // 上报抽水日志
    int  handle_msg_report_fee_log(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);   

    
    // 喇叭返回结果
    int  handle_msg_speak_broadcast(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    // 夺宝状态发送
	int  handle_msg_every_color_snatch_coin_state(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

protected:
    void NotifyLeaveState(NetworkObject* pNetObj,uint32 uid);
    
    
};

#endif // MSG_SERVER_HANDLE_H__


















