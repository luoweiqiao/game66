#ifndef _NET_H_
#define _NET_H_

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#define SOCKET_INVALID          -1
#define SOCKET_CREATE_FAILED    -2
#define SOCKET_BIND_FAILED      -3
#define SOCKET_LISTEN_FAILED    -4

class CNet
{
public: //mothed
    static int init_unix_addr(struct sockaddr_un *addr, const char *path);
    static int unix_bind (const char* path, int backlog,int flag = 0);
    static int unix_connect (int* netfd, const char* path, int block = 1, int* out_errno = NULL);
    static int init_udp_unix(const char* szPath, int bFlag);
    static int wait_udp_unix(int sock_fd, int nTimeout);
    static int tcp_bind(const char *addr, uint16_t port, int backlog);
    static int udp_bind(const char *addr, uint16_t port, int rbufsz = 0, int wbufsz = 0);
    static int tcp_connect(int* netfd, const char* address, int port,int block);
public: //property

protected: //mothed
protected: //property

private: //mothed
private: //property
};

#endif //_COMM_UNIX_SOCKET_H_

