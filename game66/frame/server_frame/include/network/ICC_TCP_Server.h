#ifndef  _ICC_TCP_SERVER_H_
#define _ICC_TCP_SERVER_H_
#include <string.h>
#include "poller.h"
#include "CCnet.h"
#include "NetworkObject.h"

class ICC_TCP_Server : public CPollerObject 
{
	public:
		ICC_TCP_Server( ){};
		virtual ~ICC_TCP_Server( ){};

		virtual void Update() = 0;

	protected:
        virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer) = 0;
};



#endif

