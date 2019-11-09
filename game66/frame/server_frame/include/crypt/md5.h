/*------------- md5.h

*/ 
/***************************************************************
* MD5加密
***************************************************************/
#ifndef __MD5_H__20160422
#define __MD5_H__20160422

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
/*************************************************************/
//------------------------------------------------------------------------------
//------------------------------------- 
class CMD5
{
private:
	typedef unsigned int (*FunT)(unsigned int x,unsigned int y,unsigned int z);

public:
	enum
	{
		MD5_LEN		 = 16		,	/*MD5后二进制长度*/ 
		MD5_STR_LEN = 32 + 1,	/*MD5后字符串长度*/ 
	};

private:
	/*定义循环数组*/ 
	static const unsigned int guArray[4][4];
	/*初始化MD5参数*/ 
	static const unsigned int A;
	static const unsigned int B;
	static const unsigned int C;
	static const unsigned int D;

private:
	unsigned int	m_guMD5[4];
	unsigned char	m_gucMD5[MD5_LEN];
	char				m_szMD5Str[MD5_STR_LEN];

	unsigned char	m_ucAppendLength;
	unsigned char	m_gucAppendBit[64];
	unsigned char	m_gucAppendLength[8];

	unsigned int	m_guTransform[16];

public:
	CMD5();
	~CMD5();

public:
	void	initMD5(const char* pBuf = NULL,unsigned int uLength = 0);

public:
	/*MD5后字符串长度*/ 
	inline char*				GetMD5Str	()	{	return m_szMD5Str;	}
	/*MD5后二进制长度*/ 
	inline unsigned char*	GetMD5		()	{	return m_gucMD5;		}

private:	
	/*四个基本的按位操作函数*/ 
	/*----->{ F(X,Y,Z) =(X&Y)|((~X)&Z) }*/ 
	static unsigned int	F(unsigned int x,unsigned int y,unsigned int z);
	/*----->{ G(X,Y,Z) =(X&Z)|(Y&(~Z)) }*/ 
	static unsigned int	G(unsigned int x,unsigned int y,unsigned int z);
	/*----->{ H(X,Y,Z) =X^Y^Z }*/ 
	static unsigned int	H(unsigned int x,unsigned int y,unsigned int z);
	/*----->{ I(X,Y,Z)=Y^(X|(~Z) }*/ 
	static unsigned int	I(unsigned int x,unsigned int y,unsigned int z);

private:	
	/*----->{ 完成四轮转换FF, GG, HH,II四个函数 }*/ 
	void	XX(FunT fun, unsigned int& a,unsigned int b,unsigned int c,unsigned int d,unsigned int mj,int s,unsigned int ti);

private:
	/*void	AppendBit(const unsigned __int64& uLength);
	void	Transform();
	void	ArrayData(const char* pBuf,const unsigned __int64& uLength);*/

private:	
	/*----->{ 补位,附加数据长度 }*/ 
	std::vector<unsigned int>	appendBit(std::vector<unsigned char> Input);

private:
	/*----->{ 对输入数据做变换 }*/ 
	void	transform(std::vector<unsigned int> m);
	/*----->{ 加密后的数据 }*/ 
	void	arrayData(std::vector<unsigned char> input);
	/*----->{ 格式化输出结果 }*/ 
	char*	formatHex(bool uppercase);

public:
	/*----->{ 得到MD5结果 }*/ 
	char*				getMD5Str(const char* pBuf,unsigned int uLength);
	/*----->{ 得到MD5结果 }*/ 
	unsigned char*	getMD5	(const char* pBuf,unsigned int uLength);
};
//------------------------------------------------------------------------------
//------------------------------------- 全局MD5字符串获取
const char*				getMD5Str(const char* password,int size);
//------------------------------------- 全局MD5编码串获取
const unsigned char* getMD5	(const char* password,int size);

#endif // __MD5_H__20160422

