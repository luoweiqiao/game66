#ifndef  _CCSocketServer_H_
#define _CCSocketServer_H_

#include "ICC_TCP_Server.h"
#include "NetworkObject.h"

class CCSocketServer  : public ICC_TCP_Server 
{
public:
	CCSocketServer(const char* bindIp, uint16_t port, int acceptcnt, int backlog);
	virtual ~CCSocketServer();

	virtual int Init();
	virtual int InputNotify();
	virtual void Update(){}

protected:
	int	_accept_cnt;
	int	_newfd_cnt;
	int* _fd_array;
	struct sockaddr_in*  _peer_array;

	uint16_t _bindPort;
	char _bindAddr[128];
	int	_backlog;

    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer) = 0;

	int proc_accept (struct sockaddr_in* peer, socklen_t* peerSize);
	int proc_request (struct sockaddr_in* peer);			
};


#endif
