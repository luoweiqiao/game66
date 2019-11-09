#include "protobuf_pkg.h"
#include "zlib.h"

using namespace svrlib;
using namespace Network;

int	  GetProtobufPacketLen(const uint8_t * pData, uint16_t wLen)
{
	if(pData == NULL){
		return -1;
	}
	if(wLen < PACKET_HEADER_SIZE){
		return -1;
	}
	packet_header_t * head = (packet_header_t *)pData;
	if(head->datalen < PACKET_MAX_DATA_SIZE)
	{
		return head->datalen + PACKET_HEADER_SIZE;
	}
	LOG_ERROR("max packet len:%d",head->datalen);
	return -1; 
}
bool  SendProtobufMsg(NetworkObject* pNetObj,const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	if(pNetObj == NULL)
	{
		return false;
	} 
	static packet_protobuf pkt;					
	memset(&pkt, 0, sizeof(pkt));
	pkt.header.cmd = msg_type;
	msg->SerializeToArray((void*)pkt.protobuf, PACKET_MAX_DATA_SIZE-1);
	pkt.header.datalen = msg->ByteSize();
    pkt.header.uin = uin;
	//LOG_DEBUG("Socket Send Msg To Client-cmd:%d--len:%d",pkt.header.cmd,pkt.header.datalen);
	return pNetObj->Send((char*)&pkt.header, pkt.header.datalen + PACKET_HEADER_SIZE);		
}

bool  SendProtobufMsg(NetworkObject* pNetObj,const void* msg, uint16 msg_len, uint16 msg_type,uint32 uin)
{
	if(pNetObj == NULL && msg_len < PACKET_MAX_DATA_SIZE-1) {
		return false;
	}
	static packet_protobuf pkt;					
	memset(&pkt, 0, sizeof(pkt));
	pkt.header.cmd = msg_type;
	memcpy((void*)pkt.protobuf, msg, msg_len);
	pkt.header.datalen = msg_len;
    pkt.header.uin = uin;
    //LOG_DEBUG("Socket Send Msg To Client-cmd:%d--len:%d",pkt.header.cmd,pkt.header.datalen);
	return pNetObj->Send((char*)&pkt.header, pkt.header.datalen + PACKET_HEADER_SIZE);	
}







