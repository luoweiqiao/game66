#ifndef _CCSocketHandler_H_
#define _CCSocketHandler_H_

#include "cache.h"
#include "ICC_Decoder.h"
#include "NetworkObject.h"
#include "protocol_base.h"
#include "MsgDecoder.h"


class CCSocketHandler : public NetworkObject
{
	public:
		CCSocketHandler();
		virtual ~CCSocketHandler();
 
	public:
		virtual int Init();
		virtual int InitSock();
		virtual int InputNotify();
		virtual int OutputNotify ();
		virtual int HangupNotify ();
		int Send(const char * buff, int len);
	
	protected:		
		virtual int OnClose() ;
		virtual int OnConnected() ;
		virtual int OnPacketComplete(char * data, int len);
        
        virtual ICC_Decoder*  CreateDecoder() { _decode = new MsgDecoder();  return _decode; }//默认的解码方式
		void Reset();

	protected:
		CConnState	_stage;

	private:
		int handle_input();
		int handle_output();

		CRawCache       _r;
		CRawCache       _w;
};

#endif

