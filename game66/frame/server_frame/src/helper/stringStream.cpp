/*------------- stringStream.cpp
* Copyright (C): 2011
* Author: toney
* Version: 1.0
* Date: 2011/8/19 12:29:13
*
*/ 
/***************************************************************
* 
***************************************************************/
#include <iostream>
#include <stdarg.h>
#include <stdlib.h>
#include "helper/stringStream.h"
#include "utility/basicFunctions.h"
#include "utility/stringFunctions.h"


/*************************************************************/
/*------------------------------------------------------------------------------
**
*/
CStringStream::CStringStream(uint32 uLen)
{
	

	m_pszBuffer	= NULL;
	m_pszPos	= NULL;
	initialize(uLen);
}
/*
**
*/
CStringStream::~CStringStream()
{
	

	if(m_pszBuffer)
		delete[] m_pszBuffer;
}

/*------------------------------------------------------------------------------
**
*/
void	CStringStream::initialize	(uint32 uLen)
{
	

	m_uLength	= uLen;
	m_pszPos	= NULL;
	if(m_pszBuffer)
		delete[] m_pszBuffer;

	if(m_uLength)
	{
		m_uLength++;
		m_pszBuffer = new char[m_uLength];
	}
	if(m_pszBuffer)
		memset(m_pszBuffer,0,m_uLength);

	m_pszPos = m_pszBuffer;
}
/*------------------------------------------------------------------------------
**
*/
void	CStringStream::reset		()
{
	

	m_pszPos	= m_pszBuffer;
}

/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::nexLine		()
{
	

	if(!m_pszPos)
		return false;

	char* pszStr = strstr(m_pszPos,"\r\n");
	if(!pszStr)
		return false;

	if((m_pszPos - m_pszBuffer) >= int32(m_uLength - 3))
		return false;

	*pszStr = '\0';
	*pszStr++ = '\0';
	m_pszPos = pszStr + 1;

	return true;
}

/*------------------------------------------------------------------------------
**
*/
char*	CStringStream::read		(const char* pszFilt)
{
	

	if(m_pszPos == m_pszBuffer + (m_uLength - 1))
		return NULL;

	char* pszStr = strstr(m_pszPos,pszFilt);
	if(!pszStr)
	{
		pszStr = m_pszPos;
		m_pszPos = m_pszBuffer + (m_uLength - 1);
		return pszStr;
	}

	*pszStr = '\0';
	char* pValue = m_pszPos;
	m_pszPos = pszStr + 1;

	return pValue;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::_read(uint32 uBytes,void* outBuffer)
{
	

	if(!uBytes || !outBuffer || getSpareSize() < uBytes)
		return false;

	memcpy(outBuffer,m_pszPos,uBytes);

	return skipPosition(uBytes);
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::_write(uint32 uBytes,const void*inBuffer)
{
	

	if(!uBytes || !inBuffer || getSpareSize() < uBytes)
		return false;

	memcpy(m_pszPos,inBuffer,uBytes);
	return skipPosition(uBytes);
}
/*------------------------------------------------------------------------------
**
*/
uint32	CStringStream::fprintf(const char* pszFormat,...)
{
	

	if(!m_pszPos || !pszFormat)
		return 0;

	va_list argptr;
	va_start(argptr, pszFormat);
	uint32 uLen = dVsprintf(m_pszPos,size_t(getSpareSize()),pszFormat,argptr);
	va_end(argptr);

	skipPosition(uLen);

	return uLen;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::setPosition(uint32 newPosition)
{
	

	if(m_uLength > newPosition)
		return false;

	m_pszPos = (m_pszBuffer + newPosition);

	return true;
}

/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read			(int32& uValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	uValue = dAtoi(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read			(uint32& uValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	uValue = dAtoi(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(int16& sValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	sValue = (int16)dAtoi(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(uint16& usValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	usValue = (uint16)dAtoi(pszStr);

	return true;
}

/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read			(int64& uValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	uValue = dAtoll(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(uint64& uValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	uValue = dAtoull(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read			(float& fValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;
	fValue = (float)atof(pszStr);

	return true;
}
/*
**
*/
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(long& lValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	lValue = dAtol(pszStr);
	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(ulong& lValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	lValue = dAtol(pszStr);
	return true;
}

/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(int8& cValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;
	cValue = (int8)dAtoi(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read		(uint8& cValue,const char* pszFilt)
{
	

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;
	cValue = (uint8)dAtoi(pszStr);

	return true;
}
/*------------------------------------------------------------------------------
**
*/
bool	CStringStream::read			(char* pszString,uint32 uLength,const char* pszFilt)
{
	

	if(!pszString || !uLength)
		return false;

	char* pszStr = read(pszFilt);
	if(!pszStr)
		return false;

	dStrcpy(pszString,uLength,pszStr);

	return true;
}


