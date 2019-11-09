#ifndef _NETWORKOBJECT_H_
#define _NETWORKOBJECT_H_

/*
namespace Network
{

class Session;
//-------------------------------------------------------------------------------------------------
/// NetworkObject
//	- fnCreateAcceptedObject 
//	- (OnAccept, OnDisconnect, OnRecv, OnConnect)
//-------------------------------------------------------------------------------------------------
class NetworkObject
{
	friend class Session;
	friend class IoHandler;

public:
	NetworkObject();
	virtual ~NetworkObject();

	void			Disconnect( int bGracefulDisconnect = true );
	bool		    Send( uint8_t *pMsg, uint16_t wSize );
	
	void			Redirect( NetworkObject *pNetworkObject );
	char*			GetIP();
	uint32_t		GetUID();
	void			SetUID(uint32_t uid);
	
private:
	virtual void	OnAccept( uint32_t dwNetworkIndex ) = 0;
	virtual void	OnDisconnect() = 0;
	virtual	int		OnRecv( uint8_t *pMsg, uint16_t wSize ) = 0;
	virtual void	OnConnect( bool bSuccess, uint32_t dwNetworkIndex ) = 0;
	virtual void	OnLogString( const char *pszLog ) = 0;
	virtual int     GetPacketLen(const uint8_t * pData, uint16_t wLen) = 0;
	virtual int		GetHeadLen() = 0;
	virtual int 	MaxTickPacket();

	inline void		SetSession( Session *pSession ) { m_pSession = pSession; }

	Session			*m_pSession;
	uint32_t		m_uid;
};
}
*/
#include "poller.h"
#include "ICC_Decoder.h"
#include <string>
using namespace std;

namespace Network
{

}

class NetworkObject : public CPollerObject
{
public:
    NetworkObject()
    {
        _enableReconnect = false;
        _decode = NULL;
        _uid = 0;
        _port = 0;
    };

    virtual ~NetworkObject(){}

public:
    virtual int OnClose() = 0;				// 连接断开后调用 注意:如果返回1则不会删除对象只会关闭连接
    virtual int OnConnected() = 0;		// 连接成功建立后调用
    virtual int OnPacketComplete(char * data, int len) = 0; // 需解析完整数据包时调用
    virtual int Send(const char * buff, int len) = 0;

    virtual NetworkObject* getObject() = 0;
    virtual int DestroyObj() { return 0; };

public:
    inline int GetIP(){ return this->_ip; };
    inline string GetSIP(){ return this->_sip; }
    inline void SetIP(int ip){ this->_ip = ip; };
    inline void SetSIP(const string & ip){ this->_sip = ip; };
    inline uint16_t GetPort(){ return this->_port; };
    inline void SetPort(uint16_t port){ this->_port = port; };

    inline int GetNetfd(){ return this->netfd; };
    inline void SetNetfd(int netfd){ this->netfd = netfd; };
    inline void EnableReconnect(){ _enableReconnect = true; }
    inline void DisableReconnect(){ _enableReconnect = false; }
    inline bool GetReconnectFlag(){ return _enableReconnect; }

    inline int GetUID() { return _uid; }
    inline void SetUID(int uid) { _uid = uid; }
protected:
    in_addr_t	_ip;
    string _sip;
    uint16_t	_port;
    bool _enableReconnect;//
    ICC_Decoder* _decode;
    int _uid;
};

#endif

