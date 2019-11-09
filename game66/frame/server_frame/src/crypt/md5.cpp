/*------------- md5.cpp
* Copyright (C): 2011
* Author: toney
* Version: 1.01
* Date: 2011/8/22 10:04:55
*
*/ 
/***************************************************************
* 
***************************************************************/
#include <assert.h>
#include "crypt/md5.h"
/*************************************************************/
//------------------------------------------------------------------------------
//------------------------------------- 定义循环数组
const unsigned int	CMD5::guArray[4][4] = { {7, 12, 17, 22}, {5, 9, 14, 20}, {4, 11, 16, 23}, {6, 10, 15, 21} };
//------------------------------------- 初始化MD5参数
const unsigned int	CMD5::A = 0x67452301;
const unsigned int	CMD5::B = 0xEFCDAB89;
const unsigned int	CMD5::C = 0x98BADCFE;
const unsigned int	CMD5::D = 0x10325476;

//------------------------------------------------------------------------------
//------------------------------------- 
CMD5::CMD5()
{
	initMD5();
}

//------------------------------------------------------------------------------
//------------------------------------- 
CMD5::~CMD5()
{
}

//------------------------------------------------------------------------------
//------------------------------------- 
void	CMD5::initMD5(const char* pBuf,unsigned int uLength)
{
	memset(m_guMD5,0,sizeof(m_guMD5));
	memset(m_gucMD5,0,sizeof(m_gucMD5));
	memset(m_szMD5Str,0,sizeof(m_szMD5Str));

	if (!pBuf || !uLength)
		return;

	std::vector<unsigned char> msg;
	for(unsigned int i = 0; i < uLength; i++)
	{
		msg.push_back(pBuf[i]);
	}
	arrayData(msg);
}

//------------------------------------------------------------------------------
//------------------------------------- 补位,附加数据长度
/*
MD5算法先对输入的数据进行补位，使得数据的长度(以unsigned char为单位)对64求余的结果是56。
即数据扩展至LEN=K*64+56个字节，K为整数。
补位方法：补一个1，然后补0至满足上述要求。相当于补一个0x80的字节，再补值
为0的字节。这一步里总共补充的字节数为0～63个。
*/
std::vector<unsigned int>	CMD5::appendBit(std::vector<unsigned char> Input)
{
	int nZeros	=	0;
	int n		=	(int)Input.size();
	int m		=	n % 64;

	if( m < 56 )
	{
		nZeros = 56 - m;
	}    
	else
	{
		nZeros = 64-m+56;
	}

	//----------补位
	if(nZeros)
	{
		Input.push_back((unsigned char)0x80);
		nZeros--;
	}
	for(int i=0;i<nZeros;i++)
	{
		Input.push_back((unsigned char)0);
	}

	//----------附加数据长度
	unsigned long long temp = n * 8;
	for( int i = 0; i < 8; i ++)
	{
		Input.push_back((unsigned char) (temp >> 8 * i) & 0xFF);	 
	}  

	//----------用一个32位的整数数组表示要变换的数据

	std::vector<unsigned int> ret;
	for (unsigned int j = 0; j < Input.size(); j += 4)
	{
		ret.push_back ( (((unsigned int)Input[j])& 0xFF) | ((((unsigned int)Input[j+1])& 0xFF) << 8) |
			((((unsigned int)Input[j+2])& 0xFF) << 16) |((((unsigned int)Input[j+3])& 0xFF) << 24) );
	}

	return ret;
}

//------------------------------------------------------------------------------
//------------------------------------- 基本的按位操作函数
//------------------------------------- F(X,Y,Z) =(X&Y)|((~X)&Z)
unsigned int	CMD5::F(unsigned int x,unsigned int y,unsigned int z)
{
	return (x&y)|((~x)&z);
}


//------------------------------------- G(X,Y,Z) =(X&Z)|(Y&(~Z))
unsigned int	CMD5::G(unsigned int x,unsigned int y,unsigned int z)
{
	return (x&z)|(y&(~z));
}

//------------------------------------- H(X,Y,Z) =X^Y^Z
unsigned int	CMD5::H(unsigned int x,unsigned int y,unsigned int z)
{
	return x^y^z;
}

//------------------------------------- I(X,Y,Z)=Y^(X|(~Z))
unsigned int	CMD5::I(unsigned int x,unsigned int y,unsigned int z)
{
	return y^(x|(~z));
}

//------------------------------------------------------------------------------
//------------------------------------- 完成四轮转换FF, GG, HH,II四个函数
/*
a = b + ((a + F(b,c,d) + X[k] + T)<<<guArray)
*/
void	CMD5::XX(CMD5::FunT fun, unsigned int& a,unsigned int b,unsigned int c,unsigned int d,unsigned int mj,int s,unsigned int ti)
{
	a = a + fun(b, c, d) + mj + ti;
	a = (a << s) | (a >> (32 - s));
	a += b;
}

//------------------------------------------------------------------------------
//------------------------------------- 对输入数据做变换
void	CMD5::transform(std::vector<unsigned int> m)
{
	unsigned int a, b, c, d;

	m_guMD5[0]	= A;
	m_guMD5[1]	= B;
	m_guMD5[2]	= C;
	m_guMD5[3]	= D;

	for(unsigned int j = 0; j < m.size() ; j += 16)
	{
		a = m_guMD5[0];
		b = m_guMD5[1];
		c = m_guMD5[2];
		d = m_guMD5[3]; 

		/* Round 1 */
		XX (&CMD5::F, a, b, c, d, m[j + 0], guArray[0][0], 0xd76aa478); /* 1 */
		XX (&CMD5::F, d, a, b, c, m[j + 1], guArray[0][1], 0xe8c7b756); /* 2 */
		XX (&CMD5::F, c, d, a, b, m[j + 2], guArray[0][2], 0x242070db); /* 3 */
		XX (&CMD5::F, b, c, d, a, m[j + 3], guArray[0][3], 0xc1bdceee); /* 4 */

		XX (&CMD5::F, a, b, c, d, m[j + 4], guArray[0][0], 0xf57c0faf); /* 5 */
		XX (&CMD5::F, d, a, b, c, m[j + 5], guArray[0][1], 0x4787c62a); /* 6 */
		XX (&CMD5::F, c, d, a, b, m[j + 6], guArray[0][2], 0xa8304613); /* 7 */
		XX (&CMD5::F, b, c, d, a, m[j + 7], guArray[0][3], 0xfd469501); /* 8 */

		XX (&CMD5::F, a, b, c, d, m[j + 8], guArray[0][0], 0x698098d8); /* 9 */
		XX (&CMD5::F, d, a, b, c, m[j + 9], guArray[0][1], 0x8b44f7af); /* 10 */
		XX (&CMD5::F, c, d, a, b, m[j +10], guArray[0][2], 0xffff5bb1); /* 11 */
		XX (&CMD5::F, b, c, d, a, m[j +11], guArray[0][3], 0x895cd7be); /* 12 */

		XX (&CMD5::F, a, b, c, d, m[j +12], guArray[0][0], 0x6b901122); /* 13 */
		XX (&CMD5::F, d, a, b, c, m[j +13], guArray[0][1], 0xfd987193); /* 14 */
		XX (&CMD5::F, c, d, a, b, m[j +14], guArray[0][2], 0xa679438e); /* 15 */
		XX (&CMD5::F, b, c, d, a, m[j +15], guArray[0][3], 0x49b40821); /* 16 */
		/* Round 2 */
		XX (&CMD5::G, a, b, c, d, m[j + 1], guArray[1][0], 0xf61e2562); /* 17 */
		XX (&CMD5::G, d, a, b, c, m[j + 6], guArray[1][1], 0xc040b340); /* 18 */
		XX (&CMD5::G, c, d, a, b, m[j +11], guArray[1][2], 0x265e5a51); /* 19 */
		XX (&CMD5::G, b, c, d, a, m[j + 0], guArray[1][3], 0xe9b6c7aa); /* 20 */

		XX (&CMD5::G, a, b, c, d, m[j + 5], guArray[1][0], 0xd62f105d); /* 21 */
		XX (&CMD5::G, d, a, b, c, m[j +10], guArray[1][1],  0x2441453); /* 22 */
		XX (&CMD5::G, c, d, a, b, m[j +15], guArray[1][2], 0xd8a1e681); /* 23 */
		XX (&CMD5::G, b, c, d, a, m[j + 4], guArray[1][3], 0xe7d3fbc8); /* 24 */

		XX (&CMD5::G, a, b, c, d, m[j + 9], guArray[1][0], 0x21e1cde6); /* 25 */
		XX (&CMD5::G, d, a, b, c, m[j +14], guArray[1][1], 0xc33707d6); /* 26 */
		XX (&CMD5::G, c, d, a, b, m[j + 3], guArray[1][2], 0xf4d50d87); /* 27 */
		XX (&CMD5::G, b, c, d, a, m[j + 8], guArray[1][3], 0x455a14ed); /* 28 */

		XX (&CMD5::G, a, b, c, d, m[j +13], guArray[1][0], 0xa9e3e905); /* 29 */
		XX (&CMD5::G, d, a, b, c, m[j + 2], guArray[1][1], 0xfcefa3f8); /* 30 */
		XX (&CMD5::G, c, d, a, b, m[j + 7], guArray[1][2], 0x676f02d9); /* 31 */
		XX (&CMD5::G, b, c, d, a, m[j +12], guArray[1][3], 0x8d2a4c8a); /* 32 */
		/* Round 3 */
		XX (&CMD5::H, a, b, c, d, m[j + 5], guArray[2][0], 0xfffa3942); /* 33 */
		XX (&CMD5::H, d, a, b, c, m[j + 8], guArray[2][1], 0x8771f681); /* 34 */
		XX (&CMD5::H, c, d, a, b, m[j +11], guArray[2][2], 0x6d9d6122); /* 35 */
		XX (&CMD5::H, b, c, d, a, m[j +14], guArray[2][3], 0xfde5380c); /* 36 */

		XX (&CMD5::H, a, b, c, d, m[j + 1], guArray[2][0], 0xa4beea44); /* 37 */
		XX (&CMD5::H, d, a, b, c, m[j + 4], guArray[2][1], 0x4bdecfa9); /* 38 */
		XX (&CMD5::H, c, d, a, b, m[j + 7], guArray[2][2], 0xf6bb4b60); /* 39 */
		XX (&CMD5::H, b, c, d, a, m[j +10], guArray[2][3], 0xbebfbc70); /* 40 */

		XX (&CMD5::H, a, b, c, d, m[j +13], guArray[2][0], 0x289b7ec6); /* 41 */
		XX (&CMD5::H, d, a, b, c, m[j + 0], guArray[2][1], 0xeaa127fa); /* 42 */
		XX (&CMD5::H, c, d, a, b, m[j + 3], guArray[2][2], 0xd4ef3085); /* 43 */
		XX (&CMD5::H, b, c, d, a, m[j + 6], guArray[2][3],  0x4881d05); /* 44 */

		XX (&CMD5::H, a, b, c, d, m[j + 9], guArray[2][0], 0xd9d4d039); /* 45 */
		XX (&CMD5::H, d, a, b, c, m[j +12], guArray[2][1], 0xe6db99e5); /* 46 */
		XX (&CMD5::H, c, d, a, b, m[j +15], guArray[2][2], 0x1fa27cf8); /* 47 */
		XX (&CMD5::H, b, c, d, a, m[j + 2], guArray[2][3], 0xc4ac5665); /* 48 */
		/* Round 4 */
		XX (&CMD5::I, a, b, c, d, m[j + 0], guArray[3][0], 0xf4292244); /* 49 */
		XX (&CMD5::I, d, a, b, c, m[j + 7], guArray[3][1], 0x432aff97); /* 50 */
		XX (&CMD5::I, c, d, a, b, m[j +14], guArray[3][2], 0xab9423a7); /* 51 */
		XX (&CMD5::I, b, c, d, a, m[j + 5], guArray[3][3], 0xfc93a039); /* 52 */

		XX (&CMD5::I, a, b, c, d, m[j +12], guArray[3][0], 0x655b59c3); /* 53 */
		XX (&CMD5::I, d, a, b, c, m[j + 3], guArray[3][1], 0x8f0ccc92); /* 54 */
		XX (&CMD5::I, c, d, a, b, m[j +10], guArray[3][2], 0xffeff47d); /* 55 */
		XX (&CMD5::I, b, c, d, a, m[j + 1], guArray[3][3], 0x85845dd1); /* 56 */

		XX (&CMD5::I, a, b, c, d, m[j + 8], guArray[3][0], 0x6fa87e4f); /* 57 */
		XX (&CMD5::I, d, a, b, c, m[j +15], guArray[3][1], 0xfe2ce6e0); /* 58 */
		XX (&CMD5::I, c, d, a, b, m[j + 6], guArray[3][2], 0xa3014314); /* 59 */
		XX (&CMD5::I, b, c, d, a, m[j +13], guArray[3][3], 0x4e0811a1); /* 60 */

		XX (&CMD5::I, a, b, c, d, m[j + 4], guArray[3][0], 0xf7537e82); /* 61 */
		XX (&CMD5::I, d, a, b, c, m[j +11], guArray[3][1], 0xbd3af235); /* 62 */
		XX (&CMD5::I, c, d, a, b, m[j + 2], guArray[3][2], 0x2ad7d2bb); /* 63 */
		XX (&CMD5::I, b, c, d, a, m[j + 9], guArray[3][3], 0xeb86d391); /* 64 */

		m_guMD5[0] = m_guMD5[0] + a;
		m_guMD5[1] = m_guMD5[1] + b;
		m_guMD5[2] = m_guMD5[2] + c;
		m_guMD5[3] = m_guMD5[3] + d;
	}	
}

//------------------------------------------------------------------------------
//------------------------------------- 加密后的数据
void	CMD5::arrayData(std::vector<unsigned char> input)
{
	std::vector<unsigned int> block = appendBit(input);
	transform(block);  
	std::vector<unsigned char> output; 
	
	for (int i = 0;i < 4;i++)
	{
		for (int j = 0;j < 4;j++)
			m_gucMD5[i * 4 + j] = (unsigned char)(m_guMD5[i] >> (j * 8) & 0xff);
	}
}

//------------------------------------------------------------------------------
//------------------------------------- 格式化输出结果
char*	CMD5::formatHex(bool uppercase)
{
	memset(m_szMD5Str,0,sizeof(m_szMD5Str));
	for (int i = 0;i < MD5_LEN;i++)
	{
#ifdef WIN32
		if (uppercase)
			sprintf_s(m_szMD5Str,sizeof(m_szMD5Str),"%s%.2X",m_szMD5Str,m_gucMD5[i]);
		else
			sprintf_s(m_szMD5Str,sizeof(m_szMD5Str),"%s%.2x",m_szMD5Str,m_gucMD5[i]);
#else//WIN32
		if (uppercase)
			sprintf(m_szMD5Str,"%s%.2X",m_szMD5Str,m_gucMD5[i]);
		else
			sprintf(m_szMD5Str,"%s%.2x",m_szMD5Str,m_gucMD5[i]);
#endif//WIN32
	}

	return m_szMD5Str;
}

//-------------------------------------------------------------
//------------------------------ 得到MD5结果
unsigned char*	CMD5::getMD5	(const char* pBuf,unsigned int uLength)
{
	initMD5(pBuf,uLength);

	return m_gucMD5;
}
//------------------------------------------------------------------------------
//------------------------------------- 得到CMD5结果
char*	CMD5::getMD5Str(const char* pBuf,unsigned int uLength)
{
	initMD5(pBuf,uLength);

	return formatHex(false);
}

///*----------------------------------------------------------------------
//* 函数名: [ CMD5::AppendBit ]
//* 说明  : {  }
//* 参数  : const unsigned __int64& uLength
//----------------------------------------------------------------------*/
//void	CMD5::AppendBit(const unsigned __int64& uLength)
//{
//	int nZeros	=	0;
//	int m		=	uLength % 64;
//
//	if( m < 56 )
//	{
//		nZeros = 56 - m;
//	}    
//	else
//	{
//		nZeros = 64 - m + 56;
//	}
//
//	m_ucAppendLength = 0;
//	memset(m_gucAppendBit,0,sizeof(m_gucAppendBit));
//	if(nZeros)
//	{
//		m_gucAppendBit[m_ucAppendLength] = 0x80;
//		nZeros--;
//	}
//	/*补0的长度*/ 
//	if (nZeros < 64)
//		m_ucAppendLength = nZeros;
//	else
//		m_ucAppendLength = 64 - 1;
//
//	unsigned __int64 temp = uLength * 8;
//	for( int i = 0; i < 8; i ++)
//		m_gucAppendLength[i] = (temp >> 8 * i) & 0xFF;
//}
//
//void	CMD5::Transform()
//{
//	unsigned int a, b, c, d;
//	a = m_guMD5[0];
//	b = m_guMD5[1];
//	c = m_guMD5[2];
//	d = m_guMD5[3]; 
//
//	/* Round 1 */
//	XX (&CMD5::F, a, b, c, d, m_guTransform[ 0], guArray[0][0], 0xd76aa478); /* 1 */
//	XX (&CMD5::F, d, a, b, c, m_guTransform[ 1], guArray[0][1], 0xe8c7b756); /* 2 */
//	XX (&CMD5::F, c, d, a, b, m_guTransform[ 2], guArray[0][2], 0x242070db); /* 3 */
//	XX (&CMD5::F, b, c, d, a, m_guTransform[ 3], guArray[0][3], 0xc1bdceee); /* 4 */
//
//	XX (&CMD5::F, a, b, c, d, m_guTransform[ 4], guArray[0][0], 0xf57c0faf); /* 5 */
//	XX (&CMD5::F, d, a, b, c, m_guTransform[ 5], guArray[0][1], 0x4787c62a); /* 6 */
//	XX (&CMD5::F, c, d, a, b, m_guTransform[ 6], guArray[0][2], 0xa8304613); /* 7 */
//	XX (&CMD5::F, b, c, d, a, m_guTransform[ 7], guArray[0][3], 0xfd469501); /* 8 */
//
//	XX (&CMD5::F, a, b, c, d, m_guTransform[ 8], guArray[0][0], 0x698098d8); /* 9 */
//	XX (&CMD5::F, d, a, b, c, m_guTransform[ 9], guArray[0][1], 0x8b44f7af); /* 10 */
//	XX (&CMD5::F, c, d, a, b, m_guTransform[10], guArray[0][2], 0xffff5bb1); /* 11 */
//	XX (&CMD5::F, b, c, d, a, m_guTransform[11], guArray[0][3], 0x895cd7be); /* 12 */
//
//	XX (&CMD5::F, a, b, c, d, m_guTransform[12], guArray[0][0], 0x6b901122); /* 13 */
//	XX (&CMD5::F, d, a, b, c, m_guTransform[13], guArray[0][1], 0xfd987193); /* 14 */
//	XX (&CMD5::F, c, d, a, b, m_guTransform[14], guArray[0][2], 0xa679438e); /* 15 */
//	XX (&CMD5::F, b, c, d, a, m_guTransform[15], guArray[0][3], 0x49b40821); /* 16 */
//	/* Round 2 */
//	XX (&CMD5::G, a, b, c, d, m_guTransform[ 1], guArray[1][0], 0xf61e2562); /* 17 */
//	XX (&CMD5::G, d, a, b, c, m_guTransform[ 6], guArray[1][1], 0xc040b340); /* 18 */
//	XX (&CMD5::G, c, d, a, b, m_guTransform[11], guArray[1][2], 0x265e5a51); /* 19 */
//	XX (&CMD5::G, b, c, d, a, m_guTransform[ 0], guArray[1][3], 0xe9b6c7aa); /* 20 */
//
//	XX (&CMD5::G, a, b, c, d, m_guTransform[ 5], guArray[1][0], 0xd62f105d); /* 21 */
//	XX (&CMD5::G, d, a, b, c, m_guTransform[10], guArray[1][1],  0x2441453); /* 22 */
//	XX (&CMD5::G, c, d, a, b, m_guTransform[15], guArray[1][2], 0xd8a1e681); /* 23 */
//	XX (&CMD5::G, b, c, d, a, m_guTransform[ 4], guArray[1][3], 0xe7d3fbc8); /* 24 */
//
//	XX (&CMD5::G, a, b, c, d, m_guTransform[ 9], guArray[1][0], 0x21e1cde6); /* 25 */
//	XX (&CMD5::G, d, a, b, c, m_guTransform[14], guArray[1][1], 0xc33707d6); /* 26 */
//	XX (&CMD5::G, c, d, a, b, m_guTransform[ 3], guArray[1][2], 0xf4d50d87); /* 27 */
//	XX (&CMD5::G, b, c, d, a, m_guTransform[ 8], guArray[1][3], 0x455a14ed); /* 28 */
//
//	XX (&CMD5::G, a, b, c, d, m_guTransform[13], guArray[1][0], 0xa9e3e905); /* 29 */
//	XX (&CMD5::G, d, a, b, c, m_guTransform[ 2], guArray[1][1], 0xfcefa3f8); /* 30 */
//	XX (&CMD5::G, c, d, a, b, m_guTransform[ 7], guArray[1][2], 0x676f02d9); /* 31 */
//	XX (&CMD5::G, b, c, d, a, m_guTransform[12], guArray[1][3], 0x8d2a4c8a); /* 32 */
//	/* Round 3 */
//	XX (&CMD5::H, a, b, c, d, m_guTransform[ 5], guArray[2][0], 0xfffa3942); /* 33 */
//	XX (&CMD5::H, d, a, b, c, m_guTransform[ 8], guArray[2][1], 0x8771f681); /* 34 */
//	XX (&CMD5::H, c, d, a, b, m_guTransform[11], guArray[2][2], 0x6d9d6122); /* 35 */
//	XX (&CMD5::H, b, c, d, a, m_guTransform[14], guArray[2][3], 0xfde5380c); /* 36 */
//
//	XX (&CMD5::H, a, b, c, d, m_guTransform[ 1], guArray[2][0], 0xa4beea44); /* 37 */
//	XX (&CMD5::H, d, a, b, c, m_guTransform[ 4], guArray[2][1], 0x4bdecfa9); /* 38 */
//	XX (&CMD5::H, c, d, a, b, m_guTransform[ 7], guArray[2][2], 0xf6bb4b60); /* 39 */
//	XX (&CMD5::H, b, c, d, a, m_guTransform[10], guArray[2][3], 0xbebfbc70); /* 40 */
//
//	XX (&CMD5::H, a, b, c, d, m_guTransform[13], guArray[2][0], 0x289b7ec6); /* 41 */
//	XX (&CMD5::H, d, a, b, c, m_guTransform[ 0], guArray[2][1], 0xeaa127fa); /* 42 */
//	XX (&CMD5::H, c, d, a, b, m_guTransform[ 3], guArray[2][2], 0xd4ef3085); /* 43 */
//	XX (&CMD5::H, b, c, d, a, m_guTransform[ 6], guArray[2][3],  0x4881d05); /* 44 */
//
//	XX (&CMD5::H, a, b, c, d, m_guTransform[ 9], guArray[2][0], 0xd9d4d039); /* 45 */
//	XX (&CMD5::H, d, a, b, c, m_guTransform[12], guArray[2][1], 0xe6db99e5); /* 46 */
//	XX (&CMD5::H, c, d, a, b, m_guTransform[15], guArray[2][2], 0x1fa27cf8); /* 47 */
//	XX (&CMD5::H, b, c, d, a, m_guTransform[ 2], guArray[2][3], 0xc4ac5665); /* 48 */
//	/* Round 4 */
//	XX (&CMD5::I, a, b, c, d, m_guTransform[ 0], guArray[3][0], 0xf4292244); /* 49 */
//	XX (&CMD5::I, d, a, b, c, m_guTransform[ 7], guArray[3][1], 0x432aff97); /* 50 */
//	XX (&CMD5::I, c, d, a, b, m_guTransform[14], guArray[3][2], 0xab9423a7); /* 51 */
//	XX (&CMD5::I, b, c, d, a, m_guTransform[ 5], guArray[3][3], 0xfc93a039); /* 52 */
//
//	XX (&CMD5::I, a, b, c, d, m_guTransform[12], guArray[3][0], 0x655b59c3); /* 53 */
//	XX (&CMD5::I, d, a, b, c, m_guTransform[ 3], guArray[3][1], 0x8f0ccc92); /* 54 */
//	XX (&CMD5::I, c, d, a, b, m_guTransform[10], guArray[3][2], 0xffeff47d); /* 55 */
//	XX (&CMD5::I, b, c, d, a, m_guTransform[ 1], guArray[3][3], 0x85845dd1); /* 56 */
//
//	XX (&CMD5::I, a, b, c, d, m_guTransform[ 8], guArray[3][0], 0x6fa87e4f); /* 57 */
//	XX (&CMD5::I, d, a, b, c, m_guTransform[15], guArray[3][1], 0xfe2ce6e0); /* 58 */
//	XX (&CMD5::I, c, d, a, b, m_guTransform[ 6], guArray[3][2], 0xa3014314); /* 59 */
//	XX (&CMD5::I, b, c, d, a, m_guTransform[13], guArray[3][3], 0x4e0811a1); /* 60 */
//
//	XX (&CMD5::I, a, b, c, d, m_guTransform[ 4], guArray[3][0], 0xf7537e82); /* 61 */
//	XX (&CMD5::I, d, a, b, c, m_guTransform[11], guArray[3][1], 0xbd3af235); /* 62 */
//	XX (&CMD5::I, c, d, a, b, m_guTransform[ 2], guArray[3][2], 0x2ad7d2bb); /* 63 */
//	XX (&CMD5::I, b, c, d, a, m_guTransform[ 9], guArray[3][3], 0xeb86d391); /* 64 */
//
//	m_guMD5[0] = m_guMD5[0] + a;
//	m_guMD5[1] = m_guMD5[1] + b;
//	m_guMD5[2] = m_guMD5[2] + c;
//	m_guMD5[3] = m_guMD5[3] + d;
//}
//
//void	CMD5::ArrayData(const char* pBuf,const unsigned __int64& uLength)
//{
//	AppendBit(uLength);
//
//	unsigned __int64 uLen = 0;
//	UINT uCount = 0;
//	for (unsigned __int64 i = 0;i < uLength + m_ucAppendLength + 8)
//	{
//		if (uLength - i > 16)
//			uCount = 16;
//		else
//			uCount = uLength - i;
//
//		if (uLength > i)
//			memcpy_s(m_guTransform,sizeof(m_guTransform),pBuf + i,uCount);
//
//		if (uLength - i > m_ucAppendLength)
//
//		Transform();
//	}
//
//}

//------------------------------------------------------------------------------
//------------------------------------- 全局MD5字符串获取
const char*	getMD5Str(const char* password,int size)
{
	static CMD5 md5;

	return md5.getMD5Str(password,size);
}
//------------------------------------- 全局MD5编码串获取
const unsigned char* getMD5(const char* password,int size)
{
	static CMD5 md5;

	return md5.getMD5(password,size);
}


