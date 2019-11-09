#pragma once

#include <string>
#include <string.h>
#include <queue>
#include <vector>
#include <deque>
#include <list>
#include <iostream>
#include <map>
#include <arpa/inet.h>

using namespace std;
typedef unsigned char BYTE;
typedef unsigned int  UINT;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef wchar_t WCHAR;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned long long _u64_;

#pragma pack(1)
struct TPkgHeader
{
    UINT        uin;         // uin(服务器内部转发用)
    USHORT      cmd;      	 // 消息id	
    USHORT      datalen;     // 消息数据长度(不包括包头)
};
#pragma pack()

#pragma pack(1)
struct PhpHeader
{
    USHORT      datalen;     // 消息数据长度(不包括包头)
    USHORT      cmd;      	 // 消息id	
};
#pragma pack()
