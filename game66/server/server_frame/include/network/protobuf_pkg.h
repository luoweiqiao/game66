
#ifndef PROTOBUF_PKG_H_
#define PROTOBUF_PKG_H_


#include <google/protobuf/stubs/common.h>
#include <google/protobuf/message.h>
#include "svrlib.h"
#include "network/NetworkObject.h"
#include "zlib.h"

using namespace svrlib;
using namespace Network;

#pragma  pack(1)
typedef struct packet_header_t {
    uint32       uin;         // uin(服务器内部转发用)  
	uint16       cmd;      	  // 消息id	
	uint16       datalen;     // 消息数据长度(不包括包头)	
}PACKETHEAD;

#define PACKET_MAX_SIZE             4096*4
#define PACKET_HEADER_SIZE          sizeof(packet_header_t)
#define PACKET_MAX_DATA_SIZE		(PACKET_MAX_SIZE - PACKET_HEADER_SIZE)

typedef struct {
	packet_header_t		header;
	uint8               protobuf[PACKET_MAX_DATA_SIZE];
} packet_protobuf;

#pragma pack()

#ifndef PARSE_MSG_FROM_ARRAY
#define PARSE_MSG_FROM_ARRAY(msg)					\
	if(!pkt_buf)									\
		return -1;									\
	if (!msg.ParseFromArray(pkt_buf, buf_len)) {	\
		LOG_ERROR("unpack fail")					\
		return -1;									\
	}												

#endif // PARSE_MSG_FROM_ARRAY

int	  GetProtobufPacketLen(const uint8_t * pData, uint16_t wLen); 
bool  SendProtobufMsg(NetworkObject* pNetObj,const google::protobuf::Message* msg,uint16 msg_type,uint32 uin = 0);
bool  SendProtobufMsg(NetworkObject* pNetObj,const void* msg, uint16 msg_len, uint16 msg_type,uint32 uin = 0);


//   消息接收器接口
class IProtobufClientMsgRecvSink
{
public:
	//  收到客户端消息时回调
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head) = 0;	

};	
class  CProtobufMsgHanlde
{
public:
	int	OnHandleClientMsg(NetworkObject* pNetObj,const uint8_t *pData, size_t uiDataLen)
	{
		if(pData == NULL)
			return -1;	
		packet_header_t * head = (packet_header_t *)pData;
		if(head->datalen > (uiDataLen-PACKET_HEADER_SIZE))
			return -1;
		if(m_pSinker != NULL){
			return m_pSinker->OnRecvClientMsg(pNetObj, pData + PACKET_HEADER_SIZE, head->datalen, head);
		}
		return -1;
	}
  	bool SetMsgSinker(IProtobufClientMsgRecvSink* pSink)
  	{
		m_pSinker = pSink;
		return true;		
  	}
	
private:
    IProtobufClientMsgRecvSink*  m_pSinker;
};














#endif // PROTOBUF_PKG_H_




