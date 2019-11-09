#ifndef _HELPER_H_
#define _HELPER_H_

#include <vector>

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/select.h>
#include <sys/types.h>
#include <asm/unistd.h>
#include <string>
#include "fundamental/common.h"

namespace svrlib
{
class CHelper
{
	typedef std::vector<unsigned char> BUF ;

public:
	static void DoMd5(const void * pBuf, size_t uiSize, uint8_t * pResult);
	static void DoSha1(const void * pBuf, size_t uiSize, uint8_t * pResult);
	static void Init_daemon(void);

	// set file limit
	static bool SetFileLimit(size_t iLimit);
	static bool GetFileSha1(std::string const& strFileName, uint8_t * pSha1, bool bSleep = false);
	static uint32_t GetDefaultPieceSize(uint64_t uiFileSize);
	static bool GetFilePieceHashs(std::string const& strFileName, std::vector<BUF>& oHash, bool bSleep = false);
	static bool GetFileSID(std::string const& strFileName, uint8_t * pSID);
	static bool GetFileFID(std::string const& strFileName, uint8_t * pFID, bool bSleep = false);


	static size_t GetDirectoryFiles(std::string const& oPathDir, std::vector<std::string>& oFileNames);
	static size_t GetAllDirectories(std::string const& oPathDir, std::vector<std::string>& oDirNames);
	static bool GetFileMd4(std::string const& strFileName, uint8_t * pMd4, bool bSleep = false);
	static std::string GetExeDir();
	static std::string GetExeFileName();
	static std::string GetLanIP();
	static std::string GetNetIP();
	static bool IsHaveNetIP();

	static bool IsLanIP(uint32_t uiIP) ;
	static size_t GetAllHostIPs(std::vector<uint32_t>& oIPs) ;


};
}

#endif

