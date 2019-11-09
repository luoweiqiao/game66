
#include <errno.h>
#include "utility/encode_utility.h"
#include <stdio.h>
#include <stdlib.h>

namespace svrlib
{

CharsetConverter::CharsetConverter(const std::string&  strFromCharset, 
				   const std::string& strToCharset)
{
#ifndef WIN32
	cd = iconv_open(strFromCharset.c_str(), strToCharset.c_str());
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CharsetConverter::~CharsetConverter()
{
#ifndef WIN32
	iconv_close(cd);
#endif
}


int CharsetConverter::Convert(iconv_t iCd,
				const std::string& strSrc,
				std::string& strResult,
				bool bErrContinue)
{
#ifndef WIN32
	unsigned int dwInLen =  strSrc.size();
	char*  pcInBuf = const_cast< char* >(strSrc.c_str());
	size_t dwInBytesLeft = dwInLen; size_t dwOutBytesLeft = dwInLen;
	char*  pcOutBuf = NULL; char *pcDest = NULL;
	size_t dwRetBytes = 0;
	int done = 0;
	int dwErrnoSaved = 0;
	int dwBufSize = dwInLen*2;
	pcDest = new char[dwBufSize];
	strResult="";
	while(dwInBytesLeft > 0 && !done)
 	{
		pcOutBuf = pcDest;
		dwOutBytesLeft = dwBufSize;
		dwRetBytes = iconv(iCd, &pcInBuf, &dwInBytesLeft, &pcOutBuf, &dwOutBytesLeft);
		dwErrnoSaved = errno;
		if (pcDest != pcOutBuf) 
	    		strResult.append(pcDest, pcOutBuf-pcDest);
		if (dwRetBytes != (size_t)-1)
	 	{
	    		pcOutBuf = pcDest;
	    		dwOutBytesLeft = dwBufSize;
	    		(void)iconv(iCd, NULL, NULL, &pcOutBuf, &dwOutBytesLeft);
	    		if (pcDest != pcOutBuf) // we have something to write
				strResult.append(pcDest, pcOutBuf-pcDest);
	    		dwErrnoSaved = 0;
	    		break;
		}
		switch(dwErrnoSaved)
		{
			// 这种情况是输出缓冲不够大
			case E2BIG: 
		    	break;
			// 这种情况是遇到无效的字符了
			case EILSEQ:
		    	if (bErrContinue) 
		    	{
					dwErrnoSaved = 0;
					dwInBytesLeft = dwInLen-(pcInBuf-strSrc.c_str()); // forward one illegal byte
					dwInBytesLeft--;
					pcInBuf++; 
			 		break;
		    	}
		     	done = 1;
		    	break;
			case EINVAL:
		    		done = 1;
		    		break;
			default:
		    		done = 1;
		    		break; 
		}
	}
	delete[] pcDest;
	errno = dwErrnoSaved;
	return (dwErrnoSaved) ? -1 : 0;
#else
	return 0 ;
#endif
}


int CharsetConverter::Convert(const std::string& strSrc,
			      std::string& strResult,
			      bool bErrContinue)
{
	return Convert(cd, strSrc, strResult, bErrContinue);
}

int CharsetConverter::Convert(const std::string& strFromCharset, const std::string& strToCharset,
	const std::string& strSrc, std::string& strResult,				
	bool bErrContinue)
{
#ifndef WIN32
	iconv_t iCd = iconv_open(strFromCharset.c_str(), strToCharset.c_str());
	if((iconv_t)-1 == iCd) 
	{
		perror("iconv_open");
		return -1;
	}


	return Convert(iCd, strSrc, strResult, bErrContinue);
#else
	return 0 ;
#endif
}

CUnicodeToGbk::CUnicodeToGbk()
{
#ifndef WIN32
	cd = iconv_open("UNICODE","GBK");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CUtf8ToGbk::CUtf8ToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK","UTF-8");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CUtf16ToGbk::CUtf16ToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK","UTF-16");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CUtf16BEToGbk::CUtf16BEToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK","UTF-16BE");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CBig5ToGbk::CBig5ToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK", "BIG5");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CISO8859ToGbk::CISO8859ToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK","ISO-8859-1");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CHzToGbk::CHzToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK","HZ");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CGbkToUtf8::CGbkToUtf8()
{
#ifndef WIN32
	cd = iconv_open("UTF-8","GBK");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}

CGb2312ToGbk::CGb2312ToGbk()
{
#ifndef WIN32
	cd = iconv_open("GBK","GB2312");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}	

CUtf8ToGb2312::CUtf8ToGb2312()
{
#ifndef WIN32
	cd = iconv_open("GB2312","UTF-8");
	if((iconv_t)-1 == cd) perror("iconv_open");
#endif
}	

} ;
