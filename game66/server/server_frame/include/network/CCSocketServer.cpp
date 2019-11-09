#include <assert.h>
#include <sys/ioctl.h>
#include "memcheck.h"
#include "CCReactor.h"
#include "CCSocketServer.h"
#include "CCSocketHandler.h"

CCSocketServer::CCSocketServer(const char* bindIp, uint16_t port, int acceptcnt, int backlog):
	_accept_cnt (acceptcnt),
	_newfd_cnt (0),
	_fd_array (NULL),
	_backlog (backlog)	
{
	strncpy (_bindAddr, bindIp, sizeof(_bindAddr) - 1);
	_bindPort = port;
	//calloc fd array
	_fd_array = (int*) CALLOC (acceptcnt, sizeof(int));
	memset (_fd_array, -1, acceptcnt * sizeof(int));
	_peer_array =  (struct sockaddr_in*) CALLOC (acceptcnt, sizeof(struct sockaddr_in));
}

CCSocketServer::~CCSocketServer()
{
	//free fd array
	FREE_CLEAR(_fd_array);
	FREE_CLEAR(_peer_array);
}
 
int CCSocketServer::Init()
{
	if((netfd = CNet::tcp_bind(_bindAddr, _bindPort, _backlog)) == -1)
	{
		//log_error ("bind addr[%s], port[%d] failed.", _bindAddr, _bindPort);
		return -1;
	}

	CPollerObject::EnableInput ();
	//log_info ("server fd[%d] bind addr[%s], port[%d] listening ...\n", netfd, _bindAddr, _bindPort);

	return 0;
}

int CCSocketServer::InputNotify(void)
{
	struct sockaddr_in*  peer = _peer_array;
	socklen_t peerSize;
	int	ret = -1;

	peerSize = sizeof (struct sockaddr_in);
	while (true)
	{
		ret = proc_accept(peer, &peerSize);
		if (_newfd_cnt <= 0)
		{
			//log_debug ("invoke proc_accept failed, new fd count = %d", _newfd_cnt);
			return POLLER_SUCC;
		}

		if (_newfd_cnt > 0)
		{
			proc_request(peer);
		}

		if (ret < 0)
		{
			return POLLER_SUCC;
		}
	}
}

int CCSocketServer::proc_accept (struct sockaddr_in* peer, socklen_t* peerSize)
{
    int newfd = -1;

    memset (_fd_array, -1, _accept_cnt * sizeof(int));
    _newfd_cnt  = 0;

	int flags = 1;

    for (int i = 0; i < _accept_cnt; ++i)
    {

		newfd = ::accept (netfd, (struct sockaddr*) &peer[i], peerSize);
		if (newfd < 0)
		{
			if (errno == EINTR)
	     	{
	        	continue;
	    	}

			if (errno == EAGAIN)
	    	{
				LOG_DEBUG("*STEP: accept new connection EAGAIN, bind[%s:%d], netfd[%d], msg[%m], errno:%d", _bindAddr, _bindPort, netfd, errno);
	       	 	return -1;
	   	 	}

			LOG_ERROR("*STEP: accept new connection failed2, bind[%s:%d], netfd[%d], msg[%m], errno:%d", _bindAddr, _bindPort, netfd, errno);
			return -1;
	    }

		if ( ioctl(newfd, FIONBIO, &flags)
			&&  ((flags = fcntl(newfd, F_GETFL, 0)) < 0
			||  fcntl(newfd, F_SETFL, flags | O_NONBLOCK) < 0) )
		{
			LOG_ERROR("operate on socket %d error connect, bind[%s:%d], errno:%d", newfd, _bindAddr, _bindPort, errno);
			close(newfd);
			continue;
		}

		LOG_DEBUG("proc_accept: accept new connection bind[%s:%d], newfd[%d], netfd[%d]", _bindAddr, _bindPort, newfd, netfd);
	    _fd_array[i] = newfd;
	    _newfd_cnt++;
	}

    return 0;
}

int CCSocketServer::proc_request (struct sockaddr_in* peer)
{
    for (int i = 0; i < _newfd_cnt; ++i)
    {
        if (_fd_array[i] == -1)
        {
            continue;
        }
        //create SocketHandle object
        NetworkObject* pTcpHandler = this->CreateHandler (_fd_array[i], &peer[i]);
        if (NULL == pTcpHandler)
        {
            //log_error ("create ICC_TCP_Handler object failed, client[%s:%d], fd[%d]", inet_ntoa(peer[i].sin_addr), peer[i].sin_port, netfd);
            ::close (_fd_array[i]);
            _fd_array[i] = -1;
            continue;
        }
		
		pTcpHandler->SetIP(peer[i].sin_addr.s_addr);
		pTcpHandler->SetSIP(inet_ntoa(peer[i].sin_addr));
		pTcpHandler->SetPort(peer[i].sin_port);
		pTcpHandler->SetNetfd(_fd_array[i]);
		
		if( pTcpHandler->OnConnected()<0 )
		{
			//log_error ("CCSocketServer::proc_request() failed, client[%s:%d], fd[%d]", inet_ntoa(peer[i].sin_addr), peer[i].sin_port, netfd);
            _fd_array[i] = -1;
			pTcpHandler->OnClose();
			//DELETE(pTcpHandler);
			pTcpHandler->DestroyObj();
            continue;
		}
		
        if( CCReactor::Instance().RegistClient(pTcpHandler) < 0 )
        {
			//log_error ("CCReactor::RegistClient failed, client[%s:%d], fd[%d]", inet_ntoa(peer[i].sin_addr), peer[i].sin_port, netfd);
            _fd_array[i] = -1;
			pTcpHandler->OnClose();
            //DELETE (pTcpHandler);//this fd is closed by ~CPollerObject() 
			pTcpHandler->DestroyObj();
			continue;
        }

		/*log_info("CCSocketServer::proc_request()--accept new connection--server_ip_port[%s:%d],client_ip_port[%s:%d], fd[%d]", 
					_bindAddr, _bindPort,inet_ntoa(peer[i].sin_addr), peer[i].sin_port ,_fd_array[i]);*/
    }
    return 0;
}
