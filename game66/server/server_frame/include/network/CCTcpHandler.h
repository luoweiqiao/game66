#ifndef __CC_TCP_HANDLE_H__
#define __CC_TCP_HANDLE_H__

#include "cache.h"
#include "ICC_Decoder.h"
#include "NetworkObject.h"
#include "protocol_base.h"
#include "MsgDecoder.h"

#define MAX_WEB_RECV_LEN            102400

class CCTcpHandle : public NetworkObject
{
public:
		CCTcpHandle();
		~CCTcpHandle();

public:
	virtual int Init();
	virtual int InputNotify();
	virtual int OutputNotify ();
	virtual int HangupNotify ();
    
    virtual void CloseConnection();
    
	int Send(const char * buff, int len);
	void Reset();
protected:		
	virtual int OnClose() ;
	virtual int OnConnected() ;
	virtual int OnPacketComplete(char * data, int len);
    virtual NetworkObject* getObject() { return this; }
	virtual ICC_Decoder*  CreateDecoder() { _decode = new MsgDecoder();  return _decode; }//默认的解码方式

public:
	int Connect();


protected:
	CConnState	_stage;

private:
	int handle_input();
	int handle_output();

	bool IsConnect;

	CRawCache       _r;
	CRawCache       _w;
};

#endif
