/*------------- bufferStream.h
* Copyright (C): 2011
* Author: toney
* Version: 1.0
* Date: 2011/8/19 12:08:30
*
*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __BUFFER_STREAM_H__
#define __BUFFER_STREAM_H__

#include "streamBase.h"
/*************************************************************/
class CBufferStream	: public CStreamBase
{
private:
	char*	m_pBuffer;		/*缓冲区指针*/ 
	uint32	m_uBufferSize;	/*缓冲区大小*/ 
	uint32	m_uBufferPos;	/*缓冲区当前位置*/ 

public:
	CBufferStream(char* pBuffer = NULL,uint32 uSize = 0);
	virtual~CBufferStream();

public:
	//--- 构建一个全局静态数据库
	static CBufferStream& buildStream();

public:
	void	initBufferStream		(char* pBuffer,uint32 uSize);

public:
	inline void		initPosition	()							{	m_uBufferPos = 0;															}
	inline char*	getBuffer		()							{	return m_pBuffer;															}
	inline char*	getSpareBuffer	()							{	return ((m_pBuffer && getSpareSize()) ? (m_pBuffer + getPosition()) : NULL);}
	inline bool		writeStream		(CBufferStream&clStream)	{	return _write(clStream.getPosition(),clStream.getBuffer());					}
	inline bool		writeStreamSpare(CBufferStream&clStream)	{	return _write(clStream.getSpareSize(),clStream.getSpareBuffer());			}

public:
	virtual uint32	getStreamSize	()							{	return m_uBufferSize;														}
	virtual uint32	getPosition		()							{	return m_uBufferPos;														}
	virtual bool	setPosition		(uint32 newPosition);

protected:
	virtual bool	_read			(uint32 uBytes,void* outBuffer);
	virtual bool	_write			(uint32 uBytes,const void*inBuffer);

public:
	virtual uint32	fprintf			(const char* pszFormat,...);

public:
	template <typename T>
	inline bool		readPointer		(T*& pPointer)
	{
		pPointer = NULL;
		if (getSpareSize() < sizeof(T))
			return false;

		pPointer = (T*)(getSpareBuffer());

		return skipPosition(sizeof(T));
	}
};

#endif // __BUFFER_STREAM_H__

