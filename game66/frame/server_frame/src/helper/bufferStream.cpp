/*------------- bufferStream.cpp
* Copyright (C): 2011
* Author: toney
* Version: 1.0
* Date: 2011/8/19 12:08:38
*
*/ 
/***************************************************************
* 
***************************************************************/
#include "utility/stringFunctions.h"
#include "helper/bufferStream.h"
#include <iostream>
#include <stdarg.h>
/*************************************************************/
//-------------------------------------------------------------
//------------------------------ 构建一个全局静态数据库
CBufferStream&CBufferStream::buildStream()
{
	static char szBuffer[1024 * 1024] = {0};
	static CBufferStream clStream;

	clStream.initBufferStream(szBuffer,sizeof(szBuffer));
	return clStream;
}
//------------------------------------------------------
//------------------------------ 
CBufferStream::CBufferStream(char* pBuffer,uint32 uSize)
{
	initBufferStream(pBuffer,uSize);
}
//------------------------------------------------------
//------------------------------ 
CBufferStream::~CBufferStream()
{
}
/*------------------------------------------------------------------------------
**
*/
void	CBufferStream::initBufferStream	(char* pBuffer,uint32 uSize)
{
	

	m_pBuffer		= pBuffer;
	m_uBufferSize	= uSize;
	m_uBufferPos	= 0;
	if (!pBuffer || !uSize)
	{
		m_pBuffer		= NULL;
		m_uBufferSize	= 0;
		return;
	}
}
bool	CBufferStream::setPosition(uint32 newPosition)
{
	

	if(newPosition <= m_uBufferSize)
	{
		m_uBufferPos=uint32(newPosition);
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------
**
*/
bool	CBufferStream::_read	(uint32 uBytes,void* outBuffer)
{
	

	if(!uBytes || !outBuffer || getSpareSize() < uBytes)
		return false;

	memcpy(outBuffer,getSpareBuffer(),uBytes);

	return skipPosition(uBytes);
}
/*------------------------------------------------------------------------------
**
*/
bool	CBufferStream::_write(uint32 uBytes,const void*inBuffer)
{
	

	if(!uBytes || !inBuffer || getSpareSize() < uBytes)
		return false;

	memcpy(getSpareBuffer(),inBuffer,size_t(uBytes));
	return skipPosition(uBytes);
}
/*------------------------------------------------------------------------------
**
*/
uint32	CBufferStream::fprintf(const char* pszFormat,...)
{
	

	if(!getSpareSize() || !pszFormat)
		return 0;

	va_list argptr;
	va_start(argptr, pszFormat);
	uint32 uLen = dVsprintf(getSpareBuffer(),size_t(getSpareSize()),pszFormat,argptr);
	va_end(argptr);

	skipPosition(uLen);

	return uLen;
}


