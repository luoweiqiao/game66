
#ifndef MSG_CLIENT_HANDLE_H__
#define MSG_CLIENT_HANDLE_H__

#include "network/protobuf_pkg.h"
#include "crypt/md5.h"

class CPlayer;
using namespace Network;

class CHandleClientMsg : public IProtobufClientMsgRecvSink
{
public: 
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);	

protected:
    // 转发给游戏服
    int  route_to_game_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
	// 转发给时时彩服务器
	int  route_to_everycolor_game_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

	// 登录 
	int  handle_msg_login(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 请求更新玩家信息
	int  handle_msg_update_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    // 请求服务器信息
	int  handle_msg_req_svrs_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 请求进入游戏服务器 
	int  handle_msg_enter_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	//进入时时彩服务器
	int  handle_msg_enter_everycolor_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	//离开时时彩服务器
	int  handle_msg_leave_everycolor_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 请求登录保险箱
	int  handle_msg_login_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 修改保险箱密码
	int  handle_msg_change_safebox_pwd(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 保险箱存取操作
	int  handle_msg_take_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 保险箱赠送操作
	int  handle_msg_give_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 获得任务奖励
	int  handle_msg_get_mission_prize(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 获得登陆奖励
	int  handle_msg_get_login_reward(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 破产补助
	int  handle_msg_get_bankrupt_help(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 兑换积分
	int  handle_msg_exchange_score(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 发送喇叭
	int  handle_msg_speak_broadcast(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 获取历史喇叭记录
	int  handle_msg_get_history_speak(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    int  handle_msg_get_server_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

	int  handle_msg_notify_vip_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

	int  handle_msg_notify_union_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_wechat_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_ali_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_other_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_qq_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_wechat_scan_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_jd_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_apple_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_large_ali_pay_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int  handle_msg_notify_large_ali_acc_recharge_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int handle_msg_notify_exclusive_alipay_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);
	int handle_msg_notify_fixed_alipay_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);
	int handle_msg_notify_fixed_wechat_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);
	int handle_msg_notify_fixed_unionpay_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);
	int handle_msg_notify_exclusive_flash_recharge_show_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);
	// 聊天信息转发
	int handle_msg_chat_info_forward_req(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);

	CPlayer* GetPlayer(NetworkObject* pNetObj);
};


#endif // MSG_CLIENT_HANDLE_H__


















