/*------------- Encrypt.h

*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __ENCRYPT_H__
#define __ENCRYPT_H__

#include "blowfish.h"
#include "xor256.h"
#include "mars.h"
#include "serpent.h"
/*************************************************************/
const int BlockSize = 16;			//我们在这里定密钥长度为128bit

class CEncrypt
{
public:
	enum EncryptEnum
	{
		Encrypt_XOR256			,		//异或简单模式
		Encrypt_BlowFish_ECB	,		//BlowFish的ECB模式
		Encrypt_BlowFish_CBC	,		//BlowFish的CBC模式
		Encrypt_BlowFish_CFB	,		//BlowFish的CFB模式
		Encrypt_AES_ECB		    ,		//AES的ECB模式
		Encrypt_AES_CBC		    ,		//AES的CBC模式
		Encrypt_AES_CFB		    ,		//AES的CFB模式
		Encrypt_MARS			,		//IBM's MARS加密算法
		Encrypt_SERPENT		    ,		//Serpent算法
		Encrypt_MaxNum
	};
private:
	EncryptEnum m_type;
	char			PublicKey[BlockSize+1];	//密钥
	char			XORKey[257];				//异或256字节密钥		

public:
	bool	makeKey	(int type, const char* KeyValue);
	bool	encrypt	(char* InString, char* OutString, int BufferLen);
	bool	decrypt	(char* InString, char* OutString, int BufferLen);		
};

#endif // __ENCRYPT_H__

