/*------------- stringStream.h
* Copyright (C): 2011
* Author: toney
* Version: 1.0
* Date: 2011/8/19 12:29:10
*
*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __STRING_STREAM_H__
#define __STRING_STREAM_H__

#include "streamBase.h"
/*************************************************************/
class CStringStream	: public CStreamBase
{
public:
	uint32	m_uLength;
	char*		m_pszBuffer;
	char*		m_pszPos;

public:
	CStringStream(uint32 uLen = 0);
	virtual~CStringStream();

public:
	inline char*&	getBuffer		()					{	return m_pszBuffer;									}
	inline uint32	getLength		()const				{	return m_uLength ? m_uLength -1 : m_uLength;		}

public:
	void			initialize		(uint32 uLen);
	void			reset			();
	bool			nexLine			();
	char*			read			(const char* pszFilt);

public:
	virtual uint32	getStreamSize	()					{	return getLength();									}
	virtual uint32	getPosition		()					{	return (m_pszPos - m_pszBuffer);					}
	virtual bool	isEof			()					{	return (m_pszPos == m_pszBuffer + (m_uLength - 1));	}
	virtual bool	setPosition		(uint32 newPosition);

protected:
	virtual bool	_read			(uint32 uBytes,void* outBuffer);
	virtual bool	_write			(uint32 uBytes,const void*inBuffer);

public:
	virtual uint32	fprintf			(const char* pszFormat,...);

public:
	bool	read	(int32& uValue	,const char* pszFilt);
	bool	read	(uint32& uValue	,const char* pszFilt);
	bool	read	(int16& sValue	,const char* pszFilt);
	bool	read	(uint16& uValue	,const char* pszFilt);
	bool	read	(int64& uValue	,const char* pszFilt);
	bool	read	(uint64& uValue	,const char* pszFilt);
	bool	read	(float& fValue	,const char* pszFilt);
	bool	read	(long& lValue	,const char* pszFilt);
	bool	read	(ulong&lValue	,const char* pszFilt);
	bool	read	(int8& cValue	,const char* pszFilt);
	bool	read	(uint8& cValue	,const char* pszFilt);
	bool	read	(char* pszString,uint32 uLength,const char* pszFilt);
};

#endif // __STRING_STREAM_H__

