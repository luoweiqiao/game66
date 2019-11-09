#include <assert.h>
#include "memcheck.h"
#include "CCReactor.h"
#include "CCTcpHandler.h"
#include <map>
#include <vector>
extern CMemPool*	_memPool;



CCTcpHandle::CCTcpHandle():
	_stage(CONN_IDLE),
	_r(*_memPool),
	_w(*_memPool)
{
	IsConnect = false;
}

CCTcpHandle::~CCTcpHandle()
{
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );

}

int CCTcpHandle::Init()
{
	if (_decode == NULL)
	{
		this->_decode = this->CreateDecoder();
	}
	assert(this->_decode);

	CPollerObject::EnableInput();
	return 0;
}

int CCTcpHandle::InputNotify(void)
{
	int stage = handle_input();
	switch (stage)
	{
		case CONN_DATA_ERROR:
			//log_error("input decode error, netfd[%d], stage[%d]", netfd, stage);
			CloseConnection();
			return POLLER_COMPLETE;

		case CONN_DISCONNECT:
			//log_error("input disconnect by user, netfd[%d], stage[%d]", netfd, stage);
			CloseConnection();
			return POLLER_COMPLETE;

		case CONN_DATA_RECVING:
		case CONN_IDLE:
		case CONN_RECV_DONE:			
			return POLLER_SUCC;

		case CONN_FATAL_ERROR:
			//log_error("input fatal error, netfd[%d], stage[%d]", netfd, stage);
			CloseConnection();
			return POLLER_COMPLETE;

		default:
			//log_error("input unknow status, netfd[%d], stage[%d]", netfd, stage);
			CloseConnection();
			return POLLER_COMPLETE;
	}
}

int CCTcpHandle::OutputNotify ()
{
	int stage = handle_output();
	switch (stage)
	{
		case CONN_SEND_DONE:
			//log_debug ("reponse data completed, netfd[%d]", netfd);
			return POLLER_COMPLETE;
		case CONN_DATA_SENDING:
			//log_debug ("reponse data sending, netfd[%d]", netfd);
			return POLLER_SUCC;
		case CONN_FATAL_ERROR:
			//log_error("reponse data CONN_FATAL_ERROR, netfd[%d]", netfd);
			CloseConnection();
			return POLLER_COMPLETE;
		default:
			//log_error("response data to client failed, netfd[%d],stage[%d]", netfd, stage);
			CloseConnection();
			return POLLER_COMPLETE;
	}
}

int CCTcpHandle::HangupNotify ()
{
	//log_info("HangupNotify fd:%d", netfd);
    
	CloseConnection();
	IsConnect = false;
    
	return POLLER_COMPLETE;
}

void CCTcpHandle::CloseConnection()
{
    this->OnClose();
	if(GetReconnectFlag()==false){
		this->DestroyObj();
	}
	else{
		Reset();
	}
}

int CCTcpHandle::handle_input()
{
	int	ret = 0 ;
	int	curr_recv_len   = 0;
	char	curr_recv_buf[MAX_WEB_RECV_LEN] = {'\0'};

	curr_recv_len = ::recv (netfd, curr_recv_buf, MAX_WEB_RECV_LEN, 0);
	int saveErrorNo = errno;//!!!该定义和上一个系统函数禁止使用log_*日志，否则打死你
	//log_debug ("*STEP: receiving data, length[%d]", curr_recv_len);

	if(-1 == curr_recv_len)
	{
		if(errno != EAGAIN && errno != EINTR && errno != EINPROGRESS)
		{
			DisableInput();
			_stage = CONN_FATAL_ERROR;
			//log_info ("recv failed from fd[%d], msg[%s]", netfd, strerror(errno));
		}
		else
			_stage = CONN_DATA_RECVING;
	}
	else if( 0 == curr_recv_len )
	{
		DisableInput ();
		_stage = CONN_DISCONNECT;
		//log_info ("CCTcpHandle::handle_input()--curr_recv_len=0,connection disconnect by user fd[%d], msg[%s]", netfd, strerror(saveErrorNo));
	}
	else
	{
		_r.append(curr_recv_buf, curr_recv_len);
		curr_recv_len = _r.data_len();
		while(_r.data_len() > 0)
		{
				int iPackageLen = this->_decode->ParsePacket(_r.data(), _r.data_len());

				if (iPackageLen < sizeof(int))
				{
					//log_debug("_r.data_len()[%d] < sizeof(int)",_r.data_len());
					_stage = CONN_DATA_RECVING;
					break;
				}

				if(iPackageLen <= 0)
				{
					_stage = CONN_RECV_DONE;
					_r.skip(curr_recv_len);
					break;
				}
				else if (curr_recv_len < iPackageLen)// || iPackageLen < (int)sizeof(int) )
				{
					//log_debug("iPackageLen[%d] >curr_recv_len[%d]",iPackageLen,curr_recv_len);
					_stage = CONN_DATA_RECVING;
					break;
				}
				
				ret = this->OnPacketComplete(_r.data(), iPackageLen);	    
				if( ret < 0 )           
				{ 
					_stage = CONN_FATAL_ERROR; 
					break;
				}
				_stage = CONN_RECV_DONE; 
				curr_recv_len = curr_recv_len - iPackageLen;
				_r.skip(iPackageLen);
		}
	}



	return _stage;
}

int CCTcpHandle::handle_output()
{
	//log_debug("*STEP: send data, len:[%d] netfd[%d]", _w.data_len(), netfd);	
	if (_w.data_len() != 0)
	{		
		int ret = ::send (netfd, _w.data(), _w.data_len(), 0);
		int saveErrorNo = errno;//!!!该定义和上一个系统函数禁止使用log_*日志，否则打死你
		if(-1 == ret)
		{
			if(saveErrorNo == EINTR || saveErrorNo == EAGAIN || saveErrorNo == EINPROGRESS)
			{
				//log_info("sending,INTR|EAGAIN|EINPROGRESS,saveErrorNo:[%d]", saveErrorNo);
				EnableOutput ();
				ApplyEvents ();
				_stage = CONN_DATA_SENDING;
				return _stage;
			}

			//log_debug ("sending package to client failed, ret[%d]",  ret);	
			DisableInput ();
			DisableOutput ();
			ApplyEvents ();
			_stage = CONN_FATAL_ERROR;
			return _stage;
		}

		if(ret == (int)_w.data_len())
		{
			//log_debug("send complete, send len=[%d]",ret);
			DisableOutput();
			ApplyEvents ();
			_w.skip(ret);	
			_stage = CONN_SEND_DONE;
			return _stage;
		}
		else if (ret < (int)_w.data_len())
		{
			//log_debug("had sent part of data, send len=[%d]",ret);
			EnableOutput ();
			ApplyEvents ();
			_w.skip(ret);
			_stage = CONN_DATA_SENDING;
			return _stage;
		}
	}



	DisableOutput();
	ApplyEvents ();	
	_stage = CONN_FATAL_ERROR;
	//log_debug("send process failure");


	return _stage;
}

int CCTcpHandle::Send(const char * buff, int len)
{
	if(len>0)
	{
		if (!IsConnect && GetReconnectFlag()==true)
		{
			//log_info("CCTcpHandle::Send()--reconnect");
			Connect();
		}

		const char* sendbuff = buff;
		int sendlen = len;

		if(this->_w.data_len()==0)
		{
			int ret = ::send (netfd,buff, len, 0);
			int saveErrorNo = errno;//!!!该定义和上一个系统函数禁止使用log_*日志，否则打死你
			if(-1 == ret)
			{
				if(saveErrorNo == EINTR || saveErrorNo == EAGAIN || saveErrorNo == EINPROGRESS)
				{
					//log_info("CCTcpHandle::Send()--sending,INTR|EAGAIN|EINPROGRESS,saveErrorNo:[%d]", saveErrorNo);
					this->_w.append(sendbuff, sendlen);
					EnableOutput ();
					ApplyEvents ();
					_stage = CONN_DATA_SENDING;
					return 0;
				}
				else
				{
					//log_error ("CCTcpHandle::Send()--sending package to client failed, ret[%d] error:%d %s",  ret, errno, strerror(errno));	
					_stage = CONN_FATAL_ERROR;
					CloseConnection();
					return -1;
				}
			}
			else if(ret<len)
			{
				sendbuff += ret;
				sendlen -=  ret;
				this->_w.append(sendbuff, sendlen);
				EnableOutput ();
				ApplyEvents ();
				_stage = CONN_DATA_SENDING;
				//log_debug("CCTcpHandle::Send()--had sent part of data, send len=[%d]",ret);
				return ret;
			}
			else if(ret==len)
			{
				//log_debug("CCTcpHandle::Send()--send complete, send len=[%d]",len);
				_stage = CONN_SEND_DONE;
				return ret;
			}
		}
		else
		{
			this->_w.append(sendbuff, sendlen);
			if( handle_output() ==CONN_FATAL_ERROR )
			{
				//log_error("fd:%d CONN_FATAL_ERROR", GetNetfd());
				CloseConnection();
				return -1;
			}
			else
				return len;
		}
	}


	return len;
}

int CCTcpHandle::OnPacketComplete(char* data, int len)
{
	return 0;
}	

int CCTcpHandle::OnClose()
{
	IsConnect = false;
	_stage = CONN_IDLE;
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );
	//log_debug(" OnClose: fd=[%d] ip=[%d:%d]",this->GetNetfd(),this->GetIP(),this->GetPort());
	return 0;
}

int CCTcpHandle::OnConnected()
{
	IsConnect = true;
	//log_debug("client  OnConnected: fd=[%d] ip=[%d:%d]",this->GetNetfd(),this->GetIP(),this->GetPort());
	return 0;
}

void CCTcpHandle::Reset(){
	//log_debug("reset socket handler");
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );
	DisableInput();
	DisableOutput();
	//ApplyEvents();
	CPollerObject::DetachPoller();
	if(netfd > 0)
	{
		::close(netfd);
	}
	netfd  = -1;
	_stage = CONN_IDLE;
	IsConnect = false;
	
	return;
}


int CCTcpHandle::Connect(){
	int ret = -1;

	if (netfd >0)
	{
		DetachPoller();
		::close(netfd);
		netfd = -1;
	}

	if (_stage == CONN_IDLE){
		ret = CNet::tcp_connect(&netfd, GetSIP().c_str(), GetPort(), 0);
	}
	else
	{
		if (netfd < 0)
		{
			ret = CNet::tcp_connect(&netfd, GetSIP().c_str(), GetPort(), 0);
		}
		else
		{
			ret = 0;
		}
	}

	int saveErrorNo = errno;//!!!该定义和上一个系统函数禁止使用log_*日志，否则打死你
	if(ret <0)
	{
		if (ret == SOCKET_CREATE_FAILED)
		{
			//log_error("*STEP: helper create socket failed, saveErrorNo[%d], msg[%s]", saveErrorNo, strerror(saveErrorNo));
			return -1;
		}

		if(saveErrorNo != EINPROGRESS)
		{
			//log_error("*STEP: PROXY connect to logic failed, saveErrorNo[%d], msg[%s]", saveErrorNo , strerror(saveErrorNo));
			return -1;
		}

		_stage = CONN_CONNECTING;

		//log_debug("*STEP: PROXY connecting to logic, unix fd[%d]", netfd);
		goto exit;
	}
	else
	{
		_stage = CONN_CONNECTED;
		IsConnect = true;
	}
exit:
	
	return CCReactor::Instance().AttachPoller(this);

}

