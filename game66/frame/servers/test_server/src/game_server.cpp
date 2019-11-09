/*
* game_server.cpp
*
*  modify on: 2015-12-2
*      Author: toney
*/
#include "svrlib.h"
#include <iostream>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include "network/zmq_pipe.h"


using namespace svrlib;
using namespace std;

int main(int argc, char * argv[])
{
	LOG_DEBUG("main start");
	CZmqPipe client;
	CZmqPipe server;

	client.Init("tcp://*:777","tcp://127.0.0.1:888");
	server.Init("tcp://*:888","tcp://127.0.0.1:777");

	while(1)
	{
		client.Tick();
		server.Tick();
		if(g_RandGen.RandUInt()%2==0) {
			string msg = CStringUtility::FormatToString("c客户端发送消息给服务器:%d", time(NULL));
			client.SendMsg(msg.data(),msg.length());
		}else{
			string msg = CStringUtility::FormatToString("s服务器发送消息给客户端:%d",time(NULL));
			server.SendMsg(msg.data(),msg.length());
		}
		sleep(1);
	}


	printf("loop end");

	sleep(2);
	return 0;
}
