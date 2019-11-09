#ifndef _BASE_HPP_
#define _BASE_HPP_

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <zlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dlfcn.h>
#include <assert.h>
#include <dirent.h>

#define FLG_CTRL_QUIT	((BYTE)0x01)
#define FLG_CTRL_RELOAD	((BYTE)0x02)
#define DIR_VERSION 10107070

#define KEYLENGTH				16		//加密键的长度
#define COMMNAMELENGTH			32		//一般名字长度
#define FILENAMELENGTH			256		//文件名的长度
#define URLLENGTH				256		//URL 长度
#define HASH_LEN                16		//HASH长度
#define MD5_HASH_LEN			16

typedef unsigned char BYTE;
typedef unsigned char byte;
typedef BYTE TKey[KEYLENGTH];
typedef char TName[COMMNAMELENGTH];
typedef char TFName[FILENAMELENGTH];

#define BIT_ENABLED(AWORD, BIT) (((AWORD) & (BIT)) != 0)
#define BIT_DISABLED(AWORD, BIT) (((AWORD) & (BIT)) == 0)
#define SET_BITS(AWORD, BITS) ((AWORD) |= (BITS))
#define CLR_BITS(AWORD, BITS) ((AWORD) &= ~(BITS))

struct PartNum32{
#if __little_endian
	int low:16;
	int high:16;
#else
	int high:16;
	int low:16;
#endif
};

union Num32{
	int num;
	struct PartNum32 part;
};

struct PartNum64{
#if __little_endian
	int low;
	int high;
#else
	int high;
	int low;
#endif
};

union Num64{
	long long num;
	struct PartNum64 part;
};

typedef enum
{
	Wrong = 0,
	False = 0,
	Free = 0,
	Right = 1,
	True  = 1,
	InUse = 1
} TBool;

typedef struct
{
	int m_iYear;
	int m_iMon;
	int m_iDay;
	int m_iHour;
	int m_iMin;
	int m_iSec;
	int m_iMSec;
} TStruTime;

class CFlag
{
public:
	CFlag();
	~CFlag();
	int Initial( BYTE byExFlag );
	BYTE GetAllFlag();
	int ClearFlag( BYTE byExFlag );
	int SetFlag( BYTE byExFlag );
	int IsFlagSet( BYTE byExFlag );
private:
	BYTE m_byFlag;
};

class CFileLock
{
public:
	CFileLock();
	~CFileLock();

	enum eParas{MAX_FILE_LOCK = 32};

	int	SetLock(int iLockIdx, const char *szFileName);
	int	DelLock(int iLockIdx);
	int	WLockWait(int iLockIdx);
	int	RLockWait(int iLockIdx);
	int	UnLock(int iLockIdx);

private:
	int m_aiLockFDs[MAX_FILE_LOCK];
};

void ConvertHashToStr(BYTE *pbyHash, char *pszHash);
int ConvertStrToHash(char *pszHash, BYTE *pbyHash);
int TimeValPlus(timeval& tvA, timeval& tvB, timeval& tvResult);
int TimeValMinus(timeval& tvA, timeval& tvB, timeval& tvResult);
int GetCurStructTime(TStruTime *pstTime);
int GetStructTime(time_t tTime, TStruTime *pstTime);
char *GetCurVersion(unsigned int unVer);

int SockAddrToString(sockaddr_in *pstSockAddr, char *szResult);
void TrimStr( char *strInput );
int SetSockBufLen(int iSock, int iLength);
int GetNowTime(time_t timer, int &nHour, int &nMinute);
int GetTimeString(time_t timer, char *strTime);
int GetDateString(char *strTime);
int SetNBlock(int iSock);

#endif
