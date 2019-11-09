/*------------- Base64.h
* Copyright (C): 2011
* Author: Mokylin
* Version: 1.01
* Date: 2011/8/22 9:50:47
*
*/ 
/***************************************************************
* 字符串base64编码
***************************************************************/
#ifndef __BASE64_H__
#define __BASE64_H__

#include <string>
/*************************************************************/
class CBase64
{
public:
	static inline bool is_base64(unsigned char c)
	{
		return (isalnum(c) || (c == '+') || (c == '/'));
	};

	static	std::string base64_encode(const char*, unsigned int len);
	static	std::string base64_encode(std::string const& s);
	static	std::string base64_decode(const char*, unsigned int len);
	static	std::string base64_decode(std::string const& s);
};

#endif // __BASE64_H__

