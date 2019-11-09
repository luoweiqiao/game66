#include <assert.h>
#include "memcheck.h"
#include "CCReactor.h"
#include "CCSocketHandler.h"

extern CMemPool*	_memPool;

CCSocketHandler::CCSocketHandler() : _stage(CONN_IDLE), _r(*_memPool), _w(*_memPool)
{

}

CCSocketHandler::~CCSocketHandler()
{
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );
	if(_decode)
		DELETE(_decode);
}


int CCSocketHandler::InitSock()
{
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );
	if(_decode)
		DELETE(_decode);

	_stage = CONN_IDLE;

	return 0;
}

int CCSocketHandler::Init()
{
	if( _decode == NULL)
	{
		this->_decode = this->CreateDecoder();
	}
	assert(this->_decode);
	CPollerObject::EnableInput ();
	return 0;
}

int CCSocketHandler::InputNotify(void)
{
	struct sockaddr_in src_addr;
	src_addr.sin_addr.s_addr = _ip;
	int stage = handle_input();

	switch (stage)
	{
	case CONN_DATA_ERROR:
		//log_error("handle_input error, netfd[%d], ip[%s], port[%d], stage[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port, stage);
		this->OnClose();
		this->DestroyObj();
		return POLLER_COMPLETE;

	case CONN_DISCONNECT:
		//log_info("handle_input disconnect by user, netfd[%d], ip[%s], port[%d], stage[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port, stage);
		this->OnClose();
		this->DestroyObj();
		return POLLER_COMPLETE;

	case CONN_DATA_RECVING:
	case CONN_IDLE:
	case CONN_RECV_DONE:			
		return POLLER_SUCC;

	case CONN_FATAL_ERROR:
		//log_error("handle_input fatal error, netfd[%d], ip[%s], port[%d], stage[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port, stage);
		this->OnClose();
		this->DestroyObj();
		return POLLER_COMPLETE;

	default:
		//log_error("handle_input default error, netfd[%d], ip[%s], port[%d], stage[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port, stage);
		this->OnClose();
		this->DestroyObj();
		return POLLER_COMPLETE;
	}
}

int CCSocketHandler::OutputNotify ()
{
	struct sockaddr_in src_addr;
	src_addr.sin_addr.s_addr = _ip;
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
		//log_error("handle_output fatal error, netfd[%d], ip[%s], port[%d], stage[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port, stage);
		this->OnClose();
		this->DestroyObj();
		return POLLER_COMPLETE;
	default:
		//log_error("handle_output default error, netfd[%d], ip[%s], port[%d], stage[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port, stage);
		this->OnClose();
		this->DestroyObj();
		return POLLER_COMPLETE;
	}
}

int CCSocketHandler::HangupNotify ()
{
	struct sockaddr_in src_addr;
	src_addr.sin_addr.s_addr = _ip;
	//log_error("HangupNotify netfd[%d], ip[%s], port[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port);

	this->OnClose();
	this->DestroyObj();

	return POLLER_COMPLETE;
}

int CCSocketHandler::handle_input()
{
	int	ret = 0 ;
	int	packet_len = 0 ;
	int	curr_recv_len   = 0;
	char	curr_recv_buf[MAX_WEB_RECV_LEN] = {'\0'};

	curr_recv_len = ::recv (netfd, curr_recv_buf, MAX_WEB_RECV_LEN, 0);
	int saveErrorNo = errno;//log_*相关函数，会修改系统errno
	
	//log_debug("CCSocketHandler::handle_input()--curr_recv_len[%d]", curr_recv_len);
	struct sockaddr_in src_addr;
	src_addr.sin_addr.s_addr = _ip;

	if(-1 == curr_recv_len)//
	{
		if (saveErrorNo == EAGAIN || saveErrorNo == EWOULDBLOCK || saveErrorNo == EINTR)
		{
			//log_info("recv from fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
			_stage = CONN_DATA_RECVING;
			return _stage;
		}

		DisableInput();
		_stage = CONN_FATAL_ERROR;
		//log_error("recv failed from fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
	}
	else if( 0 == curr_recv_len )
	{
		DisableInput ();
		_stage = CONN_DISCONNECT;

		//log_info("recv connection disconnect by user fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
	}
	else
	{
		_r.append(curr_recv_buf, curr_recv_len);
		while(_r.data_len() > 0)
		{
			packet_len = this->_decode->ParsePacket(_r.data(), _r.data_len());
			if(packet_len == -1)
			{
				DisableInput ();
				_stage = CONN_DATA_ERROR;
				break ;
			}
			else if(packet_len == 0)
			{
				_stage = CONN_DATA_RECVING;
				break;
			}
			else
			{
				ret = this->OnPacketComplete(_r.data(), packet_len);	    
				if( ret < 0 )           
				{ 
					_stage = CONN_FATAL_ERROR; 
					break;
				}
				_stage = CONN_RECV_DONE; 
				_r.skip(packet_len);
			}
		}
	}

	return _stage;
}

int CCSocketHandler::handle_output()
{
	struct sockaddr_in src_addr;
	src_addr.sin_addr.s_addr = _ip;

	if (_w.data_len() != 0)
	{		
		int ret = ::send (netfd, _w.data(), _w.data_len(), 0);
		int saveErrorNo = errno;//
		if(-1 == ret)
		{
			if(saveErrorNo == EINTR || saveErrorNo == EAGAIN || saveErrorNo == EINPROGRESS)
			{
				//log_info("send to fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
				EnableOutput ();
				ApplyEvents ();
				_stage = CONN_DATA_SENDING;
				return _stage;
			}

			//log_error("send failed fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
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
	_stage = CONN_FATAL_ERROR;
	//log_error("send process failure");
	return _stage;
}

int CCSocketHandler::Send(const char * buff, int len)
{
	struct sockaddr_in src_addr;
	src_addr.sin_addr.s_addr = _ip;

	if(len > 0)
	{
		const char* sendbuff = buff;
		int sendlen = len;

		if(this->_w.data_len() == 0)
		{
			int ret = ::send (netfd, buff, len, 0);
			int saveErrorNo = errno;//
			if(-1 == ret)
			{
				if(saveErrorNo == EINTR || saveErrorNo == EAGAIN || saveErrorNo == EINPROGRESS)
				{
					//log_info("send to fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
					this->_w.append(sendbuff, sendlen);
					EnableOutput ();
					ApplyEvents ();
					_stage = CONN_DATA_SENDING;
					return 0;
				}
				else
				{
					//log_error("send failed fd[%d], ip[%s], port[%d], msg[%s]", netfd, inet_ntoa(src_addr.sin_addr), _port, strerror(saveErrorNo));
					_stage = CONN_FATAL_ERROR;
					this->OnClose();
					this->DestroyObj();
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
				//log_debug("had sent part of data, send len=[%d]",ret);
				return ret;
			}
			else if(ret==len)
			{
				//log_debug("send complete, send len=[%d]",len);
				_stage = CONN_SEND_DONE;
				return ret;
			}
		}
		else
		{
			this->_w.append(sendbuff, sendlen);
			if( handle_output() == CONN_FATAL_ERROR )
			{
				//log_error("send handle_output failed fd[%d], ip[%s], port[%d]", netfd, inet_ntoa(src_addr.sin_addr), _port);
				this->OnClose();
				this->DestroyObj();
				return -1;
			}
			else
				return len;
		}
	}
	return len;
}

int CCSocketHandler::OnPacketComplete(char* data, int len)
{
	return 0;
}	

int CCSocketHandler::OnClose()
{
	return 0;
}

int CCSocketHandler::OnConnected()
{
	return 0;
}

void CCSocketHandler::Reset()
{
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
	return;
}
