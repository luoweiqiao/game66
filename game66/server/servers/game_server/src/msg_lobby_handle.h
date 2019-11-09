
#ifndef MSG_LOBBY_HANDLE_H__
#define MSG_LOBBY_HANDLE_H__

#include "network/protobuf_pkg.h"


using namespace Network;

class CGamePlayer;

class CHandleLobbyMsg : public IProtobufClientMsgRecvSink
{
public: 
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 游戏内消息
    virtual int  handle_msg_gameing_oper(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head) = 0;
protected:
	// 注册服务器返回
	int  handle_msg_register_rep(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 通知网络状态
    int  handle_msg_notify_net_state(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 通知修改玩家信息
    int  handle_msg_notify_change_playerinfo(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 通知服务器维护
    int  handle_msg_notify_stop_server(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 机器人离开
    int  handle_msg_leave_robot(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    int  handle_msg_flush_change_acc_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
	int  handle_msg_update_change_acc_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

	// 改变房间param
	int  handle_msg_change_room_param(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 控制玩家
	int  handle_msg_contorl_player(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 多人控制
	int  handle_msg_contorl_multi_player(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 停止夺宝
	int  handle_msg_stop_snatch_coin(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 机器人夺宝
	int  handle_msg_robot_snatch_coin(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 配置骰宝
	int  handle_msg_dice_game_control(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 配置麻将手牌
	int  handle_msg_majiang_config_hand_card(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 更新机器人配置
	int  handle_msg_update_server_room_robot(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 更新机器人配置
	int  handle_msg_reload_robot_online_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 更新新手福利权限
	int  handle_msg_update_new_player_novice_welfare_right(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 更新新手福利数值
	int  handle_msg_update_new_player_novice_welfare_value(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 刷新自动杀分配置
	int  handle_msg_update_server_auto_kill_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 刷新自动杀分玩家列表
	int  handle_msg_update_server_auto_kil_users(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
    // 刷新活跃福利配置
    int  handle_msg_update_server_active_welfare_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 重置所有玩家活跃福利信息
	int  handle_msg_reset_server_active_welfare_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 刷新新注册玩家福利配置
	int  handle_msg_update_server_new_register_welfare_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

    // 进入游戏服务器
    int  handle_msg_enter_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
	// 进入时时彩游戏服务器
	int  handle_msg_enter_every_color_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 离开时时彩游戏服务器
	int  handle_msg_leave_every_color_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

    // 提取保险箱
    int  handle_msg_take_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 兑换积分
    int  handle_msg_exchange_score(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 发送喇叭
    int  handle_msg_speak_broadcast(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    
    // 请求房间列表信息
    int  handle_msg_req_rooms_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 返回大厅
    int  handle_msg_back_lobby(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 跳转游戏服务器
    int  handle_msg_goto_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
	// 进入房间
	int  handle_msg_enter_room(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
    // 进入房间
    int  handle_msg_enter_novice_welfare_room(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 请求桌子列表
    int  handle_msg_req_table_list(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 创建桌子
    int  handle_msg_req_create_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 续费桌子
    int  handle_msg_renew_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 离开桌子
    int  handle_msg_leave_table_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 进入桌子
    int  handle_msg_enter_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 桌子准备
    int  handle_msg_table_ready(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 桌子聊天
    int  handle_msg_table_chat(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 桌子设置托管
    int  handle_msg_table_set_auto(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 快速开始
    int  handle_msg_fast_join_room(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
	
	int  handle_msg_fast_join_by_room_id(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 进入桌子
	int  handle_msg_master_join_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

    // 快速换桌
    int  handle_msg_fast_join_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 查看桌子信息
    int  handle_msg_query_table_list(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
    // 站立做起
    int  handle_msg_sitdown_standup(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
	// 道具使用
	int  handle_msg_items_user(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	// 游戏记录
	int  handle_first_game_play_log(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

	//由于配置发生改变，通知控制玩家返回大厅
	int  handle_msg_stop_control_player(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);
	//同步配置
	int  handle_msg_syn_ctrl_user_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

    //同步幸运值配置
	int  handle_msg_syn_lucky_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

	//重置幸运值配置---每天凌晨重置
	int  handle_msg_reset_lucky_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

	// 修改房间库存配置 add by har
	int handle_msg_change_room_stock_cfg(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len, PACKETHEAD* head);

	//同步捕鱼配置
	int  handle_msg_syn_fish_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head);

    CGamePlayer* GetGamePlayer(PACKETHEAD* head);
};

#endif // MSG_LOBBY_HANDLE_H__


















