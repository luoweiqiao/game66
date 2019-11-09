
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>	
#include <assert.h>
#include <errno.h>	
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <unistd.h>	
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <string.h>	

namespace Network
{
typedef int	   SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif
	
#define INVALID_SOCKET	-1
#define	SOCKET_ERROR	-1
	
#define SAFE_DELETE( p)				{ if ( p) { delete ( p); ( p) = NULL; } }
	
inline	uint32_t  GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

#define Sleep(ms)	usleep(1000 * ms)
}

#endif

