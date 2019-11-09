/**********************************************************************
*   copyright     : Tencent Innovation Center
*   version       : 0.2
*   author        : peterzhao
*   editor	  : keonxiong
*   create        : 2009-11-05
*   description   : encode utility
***********************************************************************/
#ifndef __ENCODE_UTILITY_H__
#define __ENCODE_UTILITY_H__

#ifndef WIN32
#include <iconv.h>
#endif
#include <string>
/*
功能：编码转类
参数：
	strFromCharset：源编码
	strToCharset：目标编码
	strSrc：源字符串
	strResult：目标字符串
	bErrContinue：是否忽略转换错误
	返回值：-1表示转换出错
用法：
	基类可以转换转换用户指定编码的字串
	派生类转换特定编码的字串
*/
namespace svrlib
{
class CharsetConverter
{
#ifdef WIN32
	typedef int iconv_t ;
#endif
	public:
		CharsetConverter(){}
		CharsetConverter(const std::string& strFromCharset, const std::string& strToCharset);
		virtual ~CharsetConverter();
		int Convert(const std::string& strSrc, std::string& strResult,
				bool bErrContinue = true);

		static int Convert(iconv_t iCd,
				const std::string& strSrc, std::string& strResult,
				bool bErrContinue = true);

		static int Convert(const std::string& strFromCharset, const std::string& strToCharset,
				const std::string& strSrc, std::string& strResult,
				bool bErrContinue = true);
	protected:
        iconv_t cd;
};

class CUnicodeToGbk : public CharsetConverter
{
public:
    CUnicodeToGbk();
};

class CUtf8ToGbk : public CharsetConverter
{
	public:
		CUtf8ToGbk();
};	

class CUtf16ToGbk : public CharsetConverter
{
	public:
		CUtf16ToGbk();
};


class CUtf16BEToGbk : public CharsetConverter
{
	public:
		CUtf16BEToGbk();
};

class CBig5ToGbk : public CharsetConverter
{
	public:
		CBig5ToGbk();
};

class CISO8859ToGbk : public CharsetConverter
{
	public:
		CISO8859ToGbk();
};

class CHzToGbk : public CharsetConverter
{
	public:
		CHzToGbk();
};

class CGb2312ToGbk : public CharsetConverter
{
	public:
		CGb2312ToGbk();
};

class CGbkToUtf8 : public CharsetConverter
{
	public:
		CGbkToUtf8();
};

class CUtf8ToGb2312 : public CharsetConverter
{
public:
    CUtf8ToGb2312();
};

} ;

#endif

