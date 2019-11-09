/*------------- fileStream.h
* Copyright (C): 2011
* Author: toney
* Version: 1.0
* Date: 2011/8/19 11:48:49
*
*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __FILE_STREAM_H__
#define __FILE_STREAM_H__

#include "streamBase.h"
/*************************************************************/
class CFileStream	: public CStreamBase
{
public:
	enum _enAccess
	{
		_Read		,/*读*/ 
		_Write		,/*写*/ 
		_ReadWrite	,/*读写*/ 
		_Append		,/*追加*/ 
		Access_Max
	};

protected:
	static const char	m_gszAccess[Access_Max][5];

protected:
	FILE*	m_pFile;

public:
	inline bool		isOpen	()	{	return (m_pFile != NULL);	}
	inline FILE*	getFile	()	{	return m_pFile;				}

public:
	CFileStream();
	virtual~CFileStream();

public:
	/*----->{ 打开文件 }*/ 
	bool	open			(const char*pszFileName,_enAccess eAccess);
	/*----->{ 打开文件 }*/ 
	bool	open			(const char*pszFileName,const char* pszMode = "rb");
	/*----->{ 关闭 }*/ 
	void	close			();
	//--- 更新缓冲区
	void	flush			();

public:
	/*----->{ 获得文件长度 }*/ 
	uint32	getFileLength	();
	/*----->{ 清除文件 }*/ 
	int		clear			();
	/*----->{ 偏移(成功返回0) }*/ 
	int		seek			(int32 _nOffset, int _Origin = SEEK_SET);

public:
	/*----->{ 从文件头偏移 }*/ 
	inline int	seekBegin	(int32 _nOffset)					{	return seek(_nOffset,SEEK_SET);			}
	/*----->{ 从文件尾偏移 }*/ 
	inline int	seekEnd		(int32 _nOffset)					{	return seek(_nOffset,SEEK_END);			}
	/*----->{ 从文件当前位置偏移 }*/ 
	inline int	seekCurrent	(int32 _nOffset)					{	return seek(_nOffset,SEEK_CUR);			}

public:
	virtual uint32	getStreamSize	()							{	return getFileLength();					}
	virtual uint32	getPosition		();
	virtual bool	setPosition		(uint32 newPosition)		{	return (seekBegin(newPosition) == 0);	}
	virtual bool	isEof			();
	virtual bool	skipPosition	(int32 _Bytes)				{	return (seekCurrent(_Bytes) == 0);		}

protected:
	virtual bool	_read			(uint32 uBytes,void* outBuffer);
	virtual bool	_write			(uint32 uBytes,const void*inBuffer);

public:
	virtual uint32	fread			(uint32 uMaxBytes,void* outBuffer);
	virtual uint32	fwrite			(uint32 uMaxBytes,const void*inBuffer);
	virtual uint32	fprintf			(const char* pszFormat,...);
};

#endif // __FILE_STREAM_H__

