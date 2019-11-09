/*------------- encrypt.cpp
* Copyright (C): 2011
* Author: toney
* Version: 1.01
* Date: 2011/8/22 10:38:53
*
*/ 
/***************************************************************
* 
***************************************************************/
#include "crypt/encrypt.h"
#include <stdio.h>
#include <string.h>
#ifdef WIN32
/*************************************************************/
//------------------------------------------------------------------------------
//------------------------------------- 设置密钥方法
/*
type::为算法类型
KeyValue 为密钥字符串
*/
bool CEncrypt::makeKey(int type, const char* KeyValue)
{
	if(type < 0 || type>=Encrypt_MaxNum)
		return false;

	m_type = (EncryptEnum)type;		

	if(!KeyValue || !KeyValue[0])
		return false;

	int nlen = (int)strlen(KeyValue);

	if(type== Encrypt_XOR256)		//因为异或太简单，所以密钥为2048bit
	{
		memset(XORKey,0,256);
#ifdef WIN32
		memcpy_s(XORKey,sizeof(XORKey),KeyValue,nlen);
#else//WIN32
		memcpy(XORKey,KeyValue,nlen);
#endif//WIN32
	}
	else							//密钥长度为128bit
	{		
		if( nlen>BlockSize || nlen<=0)
			return false;
		memset(PublicKey, 0,BlockSize);
#ifdef WIN32
		memcpy_s(PublicKey,sizeof(PublicKey), KeyValue,nlen);
#else//WIN32
		memcpy(PublicKey,KeyValue,nlen);
#endif//WIN32
	}
	return true;
}

//因为在下面使用IBM's Mars算法，生成Release版本，结果错误，不知为何?所以暂时屏蔽优化
#pragma optimize("", off)

//------------------------------------------------------------------------------
//------------------------------------- 加密方法
/*
InString:为待加密字符串
OutString 为加密生成的字符串
BufferLen 为字符串长度
*/
bool CEncrypt::encrypt(char* InString, char* OutString, int BufferLen)
{
	try
	{
		//异或2048位简单模式加密算法
		if(m_type == Encrypt_XOR256)
		{
			//初始化OutString
			memset(OutString,0,BufferLen);

			CXOR256 oXOR256(XORKey,256);			
			oXOR256.Crypt(InString,OutString,BufferLen);
			return true;
		}

		//BlowFish加密算法
		if(m_type == Encrypt_BlowFish_ECB || m_type==Encrypt_BlowFish_CBC || m_type==Encrypt_BlowFish_CFB)
		{			
			//如果源字符串长度小于8，直接简单异或
			if(BufferLen<8)
			{
				memcpy(OutString,InString,BufferLen);
				char* pB = OutString;
				for (int i=0; i<BufferLen; i++)
					*pB++ ^= 0x5b;
				return true;
			}

			int iType = 0;
			if(m_type == Encrypt_BlowFish_ECB)
				iType = CBlowFish::ECB;
			else if (m_type == Encrypt_BlowFish_CBC)
				iType = CBlowFish::CBC;
			else
				iType = CBlowFish::CFB;
			
			//如果待加密字符串长度不为8的倍数
			if(BufferLen%8!=0)
			{
				memcpy(OutString,InString,BufferLen);

				int nLen = BufferLen - BufferLen%8 + 1;

				char* srcStr = new char[nLen];
				char* objStr = new char[nLen];
				memset(objStr,0,nLen);

				for(int i=0 ;i<nLen;i++)
					srcStr[i] = InString != 0 ? *InString++ : 0;

				CBlowFish oBlowFish((unsigned char*)PublicKey, BlockSize);
				oBlowFish.Encrypt((unsigned char*)srcStr,(unsigned char*)objStr,nLen-1,iType);

				memcpy(OutString,objStr,nLen-1);

				delete[] srcStr;
				delete[] objStr;
			}
			else
			{
				//初始化OutString
				memset(OutString,0,BufferLen);

				CBlowFish oBlowFish((unsigned char*)PublicKey, BlockSize);
				oBlowFish.Encrypt((unsigned char*)InString,(unsigned char*)OutString,BufferLen,iType);
			}
			return true;
		}

		//AES加密算法
		if(m_type == Encrypt_AES_ECB || m_type==Encrypt_AES_CBC || m_type==Encrypt_AES_CFB)
		{
			//如果源字符串长度小于BlockSize，直接简单异或
			if(BufferLen<BlockSize)
			{
				memcpy(OutString,InString,BufferLen);
				char* pB = OutString;
				for (int i=0; i<BufferLen; i++)
					*pB++ ^= 0x6a;
				return true;
			}

			int iType = 0;
			if(m_type == Encrypt_AES_ECB)
				iType = CRijndael::ECB;
			else if (m_type == Encrypt_AES_CBC)
				iType = CRijndael::CBC;
			else
				iType = CRijndael::CFB;
			
			//如果待加密字符串长度不为BlockSize的倍数
			if(BufferLen%BlockSize!=0)
			{
				memcpy(OutString,InString,BufferLen);

				int nLen = BufferLen - BufferLen%BlockSize + 1;				

				char* srcStr = new char[nLen];
				char* objStr = new char[nLen];
				memset(objStr,0,nLen);

				for(int i=0 ;i<nLen;i++)
					srcStr[i] = InString != 0 ? *InString++ : 0;

				CRijndael oRijndael;
				oRijndael.MakeKey(PublicKey, CRijndael::sm_chain0, BlockSize, BlockSize);
				oRijndael.Encrypt(srcStr,objStr,nLen-1,iType);

				memcpy(OutString,objStr,nLen-1);

				delete[] srcStr;
				delete[] objStr;
			}
			else
			{
				//初始化OutString
				memset(OutString,0,BufferLen);

				CRijndael oRijndael;
				oRijndael.MakeKey(PublicKey, CRijndael::sm_chain0, BlockSize, BlockSize);
				oRijndael.Encrypt(InString,OutString,BufferLen,iType);
			}
			return true;
		}

		//IBM's MARS算法
		//注意：因为这里调用的Mars加密方法，
		//只能操作字符串长度为16字符串，所以对长字符串必须分段循环处理。
		if(m_type==Encrypt_MARS)
		{
			//初始化OutString
			memset(OutString,0,BufferLen);

			mars Mars;
			Mars.set_key((unsigned char*)PublicKey,16);			//设置密钥
			
			int iLen = BufferLen%16==0? BufferLen:BufferLen + 16 - BufferLen%16;
			char* Instr = new char[iLen];
			memset(Instr,0,iLen);
			memcpy(Instr,InString,BufferLen);

			int iSection = iLen / 16;
			for(int i = 0; i < iSection; i++)
			{	
				unsigned char in_block[16],out_block[16];			
				memset(in_block,0,16);
				memset(out_block,0,16);
				memcpy(in_block,Instr + 16 * i,16);
				Mars.encrypt(in_block,out_block);
				if(i != iSection -1)
					memcpy(OutString + 16 * i,out_block,16);
				else
					memcpy(OutString + 16 * i,out_block,iLen==BufferLen?16:iLen-BufferLen);
			}

			delete[] Instr;
			return true;
		}

		//Serpent算法
		//注意：因为这里调用的Serpent加密方法，
		//只能操作字符串长度为16字符串，所以对长字符串必须分段循环处理。
		if(m_type==Encrypt_SERPENT)
		{
			//初始化OutString
			memset(OutString,0,BufferLen);

			CSerpent Serpent;
			Serpent.set_key((unsigned long*)PublicKey,16);			//设置密钥

			int iLen = BufferLen%16==0? BufferLen:BufferLen + 16 - BufferLen%16;
			char* Instr = new char[iLen];
			memset(Instr,0,iLen);
			memcpy(Instr,InString,BufferLen);

			int iSection = iLen / 16;
			for(int i = 0; i < iSection; i++)
			{	
				unsigned long in_block[4],out_block[4];			
				memset(in_block,0,16);
				memset(out_block,0,16);
				memcpy(in_block,Instr + 16 * i,16);
				Serpent.encrypt(in_block,out_block);
				if(i != iSection -1)
					memcpy(OutString + 16 * i,out_block,16);
				else
					memcpy(OutString + 16 * i,out_block,iLen==BufferLen?16:iLen-BufferLen);
			}

			delete[] Instr;
			return true;
		}
	}
	catch(...)
	{
		return false;
	}
	return false;
}

//------------------------------------------------------------------------------
//------------------------------------- 解密方法
/*
InString 为待解密字符串
OutString 为解密生成的字符串
BufferLen 为字符串长度
*/
bool CEncrypt::decrypt(char* InString, char* OutString, int BufferLen)
{
	try
	{
		//异或2048位简单模式解密算法
		if(m_type == Encrypt_XOR256)
		{
			//初始化OutString
			memset(OutString,0,BufferLen);

			CXOR256 oXOR256(XORKey,256);			
			oXOR256.Crypt(InString,OutString,BufferLen);
			return true;
		}

		//BlowFish解密算法
		if(m_type == Encrypt_BlowFish_ECB || m_type==Encrypt_BlowFish_CBC || m_type==Encrypt_BlowFish_CFB)
		{

			//如果源字符串长度小于8，直接简单异或
			if(BufferLen<8)
			{
				memcpy(OutString,InString,BufferLen);
				char* pB = OutString;
				for (int i=0; i<BufferLen; i++)
					*pB++ ^= 0x5b;
				return true;
			}

			int iType = 0;
			if(m_type == Encrypt_BlowFish_ECB)
				iType = CBlowFish::ECB;
			else if (m_type == Encrypt_BlowFish_CBC)
				iType = CBlowFish::CBC;
			else
				iType = CBlowFish::CFB;
			
			//如果待解密字符串长度不为8的倍数
			if(BufferLen%8!=0)
			{
				//初始化OutString
				memcpy(OutString,InString,BufferLen);

				int nLen = BufferLen - BufferLen%8 + 1;				

				char* srcStr = new char[nLen];
				char* objStr = new char[nLen];
				memset(objStr,0,nLen);

				for(int i=0 ;i<nLen;i++)
					srcStr[i] = InString != 0 ? *InString++ : 0;

				CBlowFish oBlowFish((unsigned char*)PublicKey, BlockSize);
				oBlowFish.Decrypt((unsigned char*)srcStr,(unsigned char*)objStr,nLen-1,iType);

				memcpy(OutString,objStr,nLen-1);

				delete[] srcStr;
				delete[] objStr;
			}
			else
			{
				//初始化OutString
				memset(OutString,0,BufferLen);

				CBlowFish oBlowFish((unsigned char*)PublicKey, BlockSize);
				oBlowFish.Decrypt((unsigned char*)InString,(unsigned char*)OutString,BufferLen,iType);
			}
			return true;
		}

		//AES解密算法
		if(m_type == Encrypt_AES_ECB || m_type==Encrypt_AES_CBC || m_type==Encrypt_AES_CFB)
		{
			
			//如果源字符串长度小于BlockSize，直接简单异或
			if(BufferLen<BlockSize)
			{
				memcpy(OutString,InString,BufferLen);
				char* pB = OutString;
				for (int i=0; i<BufferLen; i++)
					*pB++ ^= 0x6a;
				return true;
			}

			int iType = 0;
			if(m_type == Encrypt_AES_ECB)
				iType = CRijndael::ECB;
			else if (m_type == Encrypt_AES_CBC)
				iType = CRijndael::CBC;
			else
				iType = CRijndael::CFB;
			
			//如果待加密字符串长度不为BlockSize的倍数
			if(BufferLen%BlockSize!=0)
			{
				memcpy(OutString,InString,BufferLen);

				int nLen = BufferLen - BufferLen%BlockSize + 1;				

				char* srcStr = new char[nLen];
				char* objStr = new char[nLen];
				memset(objStr,0,nLen);

				for(int i=0 ;i<nLen;i++)
					srcStr[i] = InString != 0 ? *InString++ : 0;

				CRijndael oRijndael;
				oRijndael.MakeKey(PublicKey, CRijndael::sm_chain0, BlockSize, BlockSize);
				oRijndael.Decrypt(srcStr,objStr,nLen-1,iType);

				memcpy(OutString,objStr,nLen-1);

				delete[] srcStr;
				delete[] objStr;
			}
			else
			{
				//初始化OutString
				memset(OutString,0,BufferLen);

				CRijndael oRijndael;
				oRijndael.MakeKey(PublicKey, CRijndael::sm_chain0, BlockSize, BlockSize);
				oRijndael.Decrypt(InString,OutString,BufferLen,iType);
			}
			return true;
		}

		//IBM's MARS算法
		//注意：因为这里调用的Mars加密方法，
		//只能操作字符串长度为16字符串，所以对长字符串必须分段循环处理。
		if(m_type==Encrypt_MARS)
		{
			mars Mars;
			Mars.set_key((unsigned char*)PublicKey,16);			//设置密钥
			
			//初始化OutString
			memset(OutString,0,BufferLen);
			
			int iLen = BufferLen%16==0? BufferLen:BufferLen + 16 - BufferLen%16;
			char* Instr = new char[iLen];
			memset(Instr,0,iLen);
			memcpy(Instr,InString,BufferLen);

			int iSection = iLen / 16;
			
			for(int i= 0; i < iSection; i++)
			{
				unsigned char in_block[16],out_block[16];
				memset(in_block,0,16);
				memset(out_block,0,16);
				memcpy(in_block,Instr + 16 * i,16);
				Mars.decrypt(in_block,out_block);
				if(i != iSection -1)
					memcpy(OutString + 16 * i,out_block,16);
				else
					memcpy(OutString + 16 * i,out_block,iLen==BufferLen?16:iLen-BufferLen);
			}

			delete[] Instr;
			return true;
		}

		//Serpent算法
		//注意：因为这里调用的Serpent解密方法，
		//只能操作字符串长度为16字符串，所以对长字符串必须分段循环处理。
		if(m_type==Encrypt_SERPENT)
		{
			//初始化OutString
			memset(OutString,0,BufferLen);

			CSerpent Serpent;
			Serpent.set_key((unsigned long*)PublicKey,16);			//设置密钥

			int iLen = BufferLen%16==0? BufferLen:BufferLen + 16 - BufferLen%16;
			char* Instr = new char[iLen];
			memset(Instr,0,iLen);
			memcpy(Instr,InString,BufferLen);

			int iSection = iLen / 16;
			for(int i = 0; i < iSection; i++)
			{	
				unsigned long in_block[4],out_block[4];			
				memset(in_block,0,16);
				memset(out_block,0,16);
				memcpy(in_block,Instr + 16 * i,16);
				Serpent.decrypt(in_block,out_block);
				if(i != iSection -1)
					memcpy(OutString + 16 * i,out_block,16);
				else
					memcpy(OutString + 16 * i,out_block,iLen==BufferLen?16:iLen-BufferLen);
			}

			delete[] Instr;
			return true;
		}
	}
	catch(...)
	{
		return false;
	}
	return false;
}
#pragma optimize("", on)
#endif//WIN32


