

#include "helper/helper.h"
#include <sstream>
#include <iomanip>

#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include "hash/sha1.h"
#include "helper/filehelper.h"
#include "hash/md4emule.h"
#include "hash/md5.h"
#include "utility/timeutility.h"


using namespace svrlib ;

static const unsigned int SLEEP_INTERVAL = 5;

void CHelper::DoMd5(const void * pBuf, size_t uiSize, uint8_t * pResult)
{
	md5_t md5InfoBuffer;
	md5_init(&md5InfoBuffer);
	md5_process(&md5InfoBuffer, pBuf, uiSize);
	md5_finish(&md5InfoBuffer, pResult);
}



int GenCoreDumpFile(size_t size = 1024 * 1024 * 32)
{
	struct rlimit flimit;
	flimit.rlim_cur=size;
	flimit.rlim_max=size;
	if(setrlimit(RLIMIT_CORE,&flimit) != 0)
	{	
		return errno;
	}

	return 0;
}

//守护进程初始化函数 
void
CHelper::Init_daemon(void)
{

    char szCurDir[1024] =
        { 0 };
    getcwd(szCurDir, sizeof(szCurDir));

    int pid;
    if ((pid = fork()) > 0)
        exit(0);//是父进程，结束父进程
    else if (pid < 0)
        exit(1);//fork失败，退出
    //是第一子进程，后台继续执行

    setsid();//第一子进程成为新的会话组长和进程组长

    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    struct sigaction sig;

    sig.sa_handler = SIG_IGN;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGHUP, &sig, NULL);
    //并与控制终端分离
    if ((pid = fork()) > 0)
    {
        exit(0);//是第一子进程，结束第一子进程
    }
    else if (pid < 0)
    {
        exit(1);//fork失败，退出
    }
    //是第二子进程，继续
    //第二子进程不再是会话组长

    //for(i=0;i<NOFILE;++i)//关闭打开的文件描述符
    //	close(i);
    chdir(szCurDir);//改变工作目录到/tmp
    //umask(0);//重设文件创建掩模
    GenCoreDumpFile((uint32_t) (1024UL * 1024 * 1024 * 2));
    return;

}


bool CHelper::SetFileLimit(const size_t iLimit)
{
	bool bOK = false;

	struct rlimit rlim = {0,0};
	if(getrlimit((__rlimit_resource_t)RLIMIT_OFILE, &rlim) == 0)
	{
		//		int cur = rlim.rlim_cur;
		if(rlim.rlim_cur < iLimit)
		{
			rlim.rlim_cur = iLimit;
			rlim.rlim_max = iLimit;
			if(setrlimit((__rlimit_resource_t)RLIMIT_OFILE, &rlim) == 0)
			{
				bOK = true;
			}
		}
		else
		{
			bOK = true;
		}
	}
	return bOK;
}


bool CHelper::GetFileSha1(std::string const& strFileName, uint8_t * pSha1, bool bSleep)
{
	CFileHelper oFile(strFileName.c_str(), CFileHelper::MOD_RDONLY) ;
	if (!oFile.IsOpen())
	{
		return false ;
	}
	enum {BLOCK_SIZE = 4 * 1024 * 1024};
	SHA_INFO oInfo;
	sha_init(&oInfo);
	//unsigned char arBuf[BLOCK_SIZE] ;
	BUF arBuf(BLOCK_SIZE) ;
	unsigned int uiReadSize = 0 ;
	uint64_t iOff = 0 ;
	unsigned int uiTimes = 0 ;
	do
	{
		uiReadSize = oFile.Read(iOff, &arBuf[0], BLOCK_SIZE) ;
		iOff += uiReadSize ;
		if (uiReadSize > 0)
		{
			sha_update(&oInfo, &arBuf[0], uiReadSize) ;
		}
		if (bSleep)
		{
			++uiTimes ;
			if (uiTimes % SLEEP_INTERVAL == 0)
			{
				CTimeUtility::Sleep(2) ;
			}
		}
	} while(uiReadSize == BLOCK_SIZE) ;
	sha_final(pSha1, &oInfo) ;
	return true ;
}

uint32_t CHelper::GetDefaultPieceSize(uint64_t uiFileSize)
{
	const uint32_t K = 1024;
	const uint32_t M = K * K;
	//__int64 uiFileSizeM = uiFileSize / M;
	if (uiFileSize <= 2 * M) return 32 * K;
	else if (uiFileSize <= 20 * M) return 64 * K;
	else if (uiFileSize <= 200 * M) return 128 * K ;
	else if (uiFileSize <= 1024 * M) return 256 * K ;
	else if (uiFileSize <= (uint64_t)4096 * M) return 512 * K;
	else return (uint32_t)((uiFileSize + 8192 * K - 1) / (8192 * K) * K);
}

bool CHelper::GetFilePieceHashs(std::string const& strFileName, std::vector<BUF>& oHash, bool bSleep)
{
	CFileHelper oFile(strFileName.c_str(), CFileHelper::MOD_RDONLY) ;
	if (!oFile.IsOpen())
	{
		return false ;
	}
	uint32_t uiPieceSize = GetDefaultPieceSize(CFileHelper::GetFileSize(strFileName)) ;
	oHash.clear() ;
	BUF oReadBuf(uiPieceSize) ;
	BUF oMd5Buf(16) ;
	size_t uiReadSize = 0 ;
	uint64_t iOff = 0 ;
	unsigned int uiTimes = 0 ;
	do
	{
		uiReadSize = oFile.Read(iOff, &oReadBuf[0], oReadBuf.size()) ;
		iOff += uiReadSize ;
		if (uiReadSize > 0)
		{
			DoMd5(&oReadBuf[0], uiReadSize, &oMd5Buf[0]) ;
			oHash.push_back(oMd5Buf) ;
		}
		if (bSleep)
		{
			++uiTimes ;
			if (uiTimes % SLEEP_INTERVAL == 0)
			{
				CTimeUtility::Sleep(2) ;
			}
		}
	} while(uiReadSize == uiPieceSize) ;
	return true ;
}

bool CHelper::GetFileSID(std::string const& strFileName, uint8_t * pSID)
{
	CFileHelper oFile(strFileName.c_str(), CFileHelper::MOD_RDONLY) ;
	if (!oFile.IsOpen())
	{
		return false ;
	}
	uint64_t uiFileSize = CFileHelper::GetFileSize(strFileName) ;
	if (uiFileSize == 0)
	{
		return false ;
	}
	unsigned int uiPieceSize = GetDefaultPieceSize(uiFileSize) ;
	unsigned int uiPieceCount = ((uint64_t)uiFileSize + uiPieceSize - 1) / uiPieceSize ;
	BUF oReadBuf(uiPieceSize) ;
	unsigned int uiReadSize = 0 ;
	unsigned char arMd5[16] ;
	SHA_INFO oInfo;
	sha_init(&oInfo);
	
	if (uiPieceCount == 1)
	{
		uiReadSize = oFile.Read(0, &oReadBuf[0], (unsigned int)oReadBuf.size()) ;
		sha_update(&oInfo, &oReadBuf[0], oReadBuf.size()) ;
	}
	else if (uiPieceCount == 2)
	{
		uiReadSize = oFile.Read(0, &oReadBuf[0], oReadBuf.size()) ;
		DoMd5(&oReadBuf[0], uiReadSize, arMd5) ;
		sha_update(&oInfo, arMd5, 16) ;

		uiReadSize = oFile.Read(uiPieceSize, &oReadBuf[0], oReadBuf.size()) ;
		DoMd5(&oReadBuf[0], uiReadSize, arMd5) ;
		sha_update(&oInfo, arMd5, 16) ;
	}
	else
	{
		uiReadSize = oFile.Read(0, &oReadBuf[0], oReadBuf.size()) ;
		DoMd5(&oReadBuf[0], uiReadSize, arMd5) ;
		sha_update(&oInfo, arMd5, 16) ;

		unsigned int iOff = (uiPieceCount / 2) * uiPieceSize ;
		uiReadSize = oFile.Read(iOff, &oReadBuf[0], oReadBuf.size()) ;
		DoMd5(&oReadBuf[0], uiReadSize, arMd5) ;
		sha_update(&oInfo, arMd5, 16) ;

		iOff = (uiPieceCount - 1) * uiPieceSize ;
		uiReadSize = oFile.Read(iOff, &oReadBuf[0], oReadBuf.size()) ;
		DoMd5(&oReadBuf[0], uiReadSize, arMd5) ;
		sha_update(&oInfo, arMd5, 16) ;
	}

	uint64_t i64FileSize = uiFileSize ;
	sha_update(&oInfo, (uint8_t *)&i64FileSize, sizeof(i64FileSize)) ;
	sha_final(pSID, &oInfo) ;
	return true ;
}

bool CHelper::GetFileFID(std::string const& strFileName, uint8_t * pFID, bool bSleep)
{
	std::vector<BUF> oHashs ;
	if (!GetFilePieceHashs(strFileName, oHashs, bSleep))
	{
		return false ;
	}
	SHA_INFO oInfo;
	sha_init(&oInfo);

	CFileHelper oFile(strFileName.c_str(), CFileHelper::MOD_RDONLY) ;
	uint64_t i64FileSize = CFileHelper::GetFileSize(strFileName) ;
	sha_update(&oInfo, (unsigned char *)&i64FileSize, sizeof(i64FileSize)) ;
	for (size_t i = 0; i < oHashs.size(); ++i)
	{
		sha_update(&oInfo, &oHashs[i][0], oHashs[i].size()) ;
	}
	sha_final(pFID, &oInfo) ;
	return true ;
}


void CHelper::DoSha1(const void * pBuf, size_t uiSize, unsigned char * pResult)
{
	SHA_INFO oInfo;
	sha_init(&oInfo);
	sha_update(&oInfo, (unsigned char *)pBuf, uiSize) ;
	sha_final(pResult, &oInfo) ;
}



size_t CHelper::GetDirectoryFiles(std::string const& oPathDir, std::vector<std::string>& oFileNames)
{
	oFileNames.clear() ;


	std::string oPath = oPathDir ;
	if (oPath.empty())
	{
		return 0;
	}
	if (oPath[oPath.size() - 1] != '/')
	{
		oPath += "/" ;
	}

	DIR * dp; 
	if( (dp = opendir(oPath.c_str())) == NULL)
	{
		return 0 ;
	}
	struct dirent * dirp;
	while((dirp = readdir(dp)) != NULL)   
	{   
		struct dirent temp = (*dirp) ;
		struct stat buf;  
		std::string oTempPath = oPath ;
		oTempPath += temp.d_name ; 
		if (lstat(oTempPath.c_str(),  &buf) < 0)   
		{   
			continue ;
		}   
		if (S_ISDIR(buf.st_mode))   
		{   
			continue ;
		}
		else
		{
			oFileNames.push_back(temp.d_name) ;
		}
	}   
	closedir(dp);  
	return oFileNames.size() ;

}

size_t CHelper::GetAllDirectories(std::string const& oPathDir, std::vector<std::string>& oDirNames)
{
	oDirNames.clear() ;


	std::string oPath = oPathDir ;
	if (oPath.empty())
	{
		return 0;
	}
	if (oPath[oPath.size() - 1] != '/')
	{
		oPath += "/" ;
	}

	DIR * dp; 
	if( (dp = opendir(oPath.c_str())) == NULL)
	{
		return 0 ;
	}
	struct dirent * dirp;
	while((dirp = readdir(dp)) != NULL)   
	{   
		struct dirent temp = (*dirp) ;
		struct stat buf;  
		std::string oTempPath = oPath ;
		oTempPath += temp.d_name ; 
		if (lstat(oTempPath.c_str(),  &buf) < 0)   
		{   
			continue ;
		}   
		std::string strDirName = temp.d_name ;
		if (S_ISDIR(buf.st_mode) && strDirName != "." && strDirName != "..")   
		{   
			oDirNames.push_back(temp.d_name) ;
		}
	}   
	closedir(dp);  
	return oDirNames.size() ;

}


#define EMULE_MD4_BLOCKSIZE	(9500*1024)


#define SLASH_CHAR	'/'


struct Md4Info
{

	unsigned int	 blocks;
	unsigned char	*parthashes;		/* the partial hashes in a row		*/
	unsigned char	*ed2k_hash;			/* the ed2k_hash in binary form		*/
	uint64_t			 size;				/* the filesize in bytes			*/
} ;

bool CHelper::GetFileMd4(std::string const& strFileName, uint8_t * pMd4, bool bSleep)
{
	unsigned int	b;
	Md4Info		fi;
	bool bFail = false ;
	memset (&fi,0x00, sizeof(fi));
	
	CFileHelper oFile(strFileName.c_str(), CFileHelper::MOD_RDONLY) ;
	if (!oFile.IsOpen())
	{
		return false ;
	}
	fi.size = CFileHelper::GetFileSize(strFileName) ;
	if (fi.size == (uint64_t)(CFileHelper::ERR_SIZE))
	{
		return false ;
	}
	//fi.filepath = oFileName.szFileName;

	fi.blocks = (fi.size + EMULE_MD4_BLOCKSIZE - 1) / EMULE_MD4_BLOCKSIZE;

	fi.ed2k_hash = (unsigned char*) malloc(16);
	fi.parthashes = (unsigned char*) malloc(fi.blocks*16);	/* 16 bytes for each block's hash */

	if (fi.parthashes && fi.ed2k_hash)
	{
		for (b=0; b < fi.blocks && !bFail; b++)
		{
			MD4_CTX			 context;
			unsigned int	 blocksize = EMULE_MD4_BLOCKSIZE;
			unsigned int	 left;
			unsigned char	 hash[16];

			if (oFile.Seek((uint64_t)b * EMULE_MD4_BLOCKSIZE))
			{
				if (b == fi.blocks - 1)
				{
					blocksize = fi.size % EMULE_MD4_BLOCKSIZE;
					if (!blocksize)
					{
						blocksize = EMULE_MD4_BLOCKSIZE;
					}
				}
				
				left = blocksize;
				MD4Init (&context);
				while (left>0)
				{
					unsigned char	buf[4*1024];
					size_t			readnow = (left>=sizeof(buf)) ? sizeof(buf) : left;
					int iRead = oFile.Read(buf, readnow) ;//_read(ieMule, buf, readnow);
					if ((size_t)iRead != readnow)
					{
						bFail = true ;
						break;
					}
					
					MD4Update (&context, buf, readnow);
					left -= readnow;
				}
				
				if (bSleep)
				{
					CTimeUtility::Sleep(1); // 休息一下，防止cpu耗尽
				}
				
				MD4Final (hash, &context);
				memcpy (fi.parthashes+(b*16), hash, 16);
				
			}
		}

		/* if only one block: partial hash == final hash */
		if (fi.blocks>1)
		{
			MD4_CTX	context;
			MD4Init (&context);
			MD4Update (&context, fi.parthashes, 16*b);
			MD4Final (fi.ed2k_hash, &context);
		} 
		else 
		{
			memcpy (fi.ed2k_hash, fi.parthashes, 16);
		}

		memcpy(pMd4, fi.ed2k_hash, 16);
	}	

	free(fi.ed2k_hash);
	free(fi.parthashes);
	return !bFail ;
}

std::string CHelper::GetExeDir()
{

	const int MAXBUFSIZE = 1024;

	char szFileName[MAXBUFSIZE] = { 0 };

	int count;
	count = readlink("/proc/self/exe", szFileName, sizeof(szFileName));
	char * p = strrchr(szFileName, '/');
	if (p)
		*(p + 1) = 0;
	return szFileName;

}

std::string CHelper::GetExeFileName()
 {
	const int MAXBUFSIZE = 1024;
	char szFileName[MAXBUFSIZE] = { 0 };

	int count;
	count = readlink("/proc/self/exe", szFileName, sizeof(szFileName));
	return szFileName;

}


struct stIP
{
	union
	{
		uint32_t uiIP ;
		uint8_t arIP[4] ;
	} ;
} ;


bool CHelper::IsLanIP(uint32_t uiIP)
{
	stIP oIP ;
	oIP.uiIP = uiIP ;
	if (oIP.arIP[0] == 10) // 10.0.0.0 - 10.255.255.255
	{
		return true ;
	}
	if (oIP.arIP[0] == 172 && (oIP.arIP[1] >= 16 && oIP.arIP[1] <= 31))
	{
		return true ;
	}
	if (oIP.arIP[0] == 192 && oIP.arIP[1] == 168) 
	{
		return true ;
	}
	if (oIP.arIP[0] == 169 && oIP.arIP[1] == 254)
	{
		return true ;
	}
	return false ;
}

size_t CHelper::GetAllHostIPs(std::vector<uint32_t>& oIPs)
{

	enum
	{
		MAXINTERFACES = 16,
	} ;
	int fd = 0;
	int intrface = 0 ;
	struct ifreq  buf[MAXINTERFACES];
	struct ifconf ifc;  
	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		ifc.ifc_len = sizeof(buf);               
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
		{
			intrface = ifc.ifc_len / sizeof(struct ifreq);
			while (intrface-- > 0)
			{
				if(!(ioctl(fd, SIOCGIFADDR, (char*)&buf[intrface])))       
				{       
					uint32_t uiIP = ((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr.s_addr ;
					if (uiIP != 0 && uiIP != inet_addr("127.0.0.1"))
					{
						oIPs.push_back(uiIP) ;
					}
				}       
			}
		}
	}
	close(fd);

	return oIPs.size() ;
}

std::string CHelper::GetLanIP()
{
	std::vector<uint32_t> oIPs ;
	if (GetAllHostIPs(oIPs) > 0)
	{
		for (size_t i = 0; i < oIPs.size(); ++i)
		{
			if (IsLanIP(oIPs[i]))
			{
			    struct in_addr stAddr;
			    stAddr.s_addr = oIPs[i];
				return inet_ntoa(stAddr) ;
			}
		}
	}
	// 返回无效的IP
	return "1.1.0.1" ;
}

std::string CHelper::GetNetIP()
{
    uint32_t uiNetIP = 0 ;
	std::vector<uint32_t> oIPs ;
	if (GetAllHostIPs(oIPs) > 0)
	{
		for (size_t i = 0; i < oIPs.size(); ++i)
		{
			if (!IsLanIP(oIPs[i]))
			{
				if (uiNetIP == 0)
				{
					uiNetIP = oIPs[i] ;
				}
				else
				{
					// 如果有多个外网ip，则返回ip 0					
					break;
				}
			}
		}
	}
	struct in_addr stAddr;
	stAddr.s_addr = uiNetIP;
	return inet_ntoa(stAddr) ;
}
bool CHelper::IsHaveNetIP()
{
	std::vector<uint32_t> oIPs;
	if(GetAllHostIPs(oIPs) > 0)
	{
		for(size_t i = 0; i < oIPs.size(); ++i)
		{
			if(!IsLanIP(oIPs[i])){
				return true;
			}
		}
	}
	return false;
}

