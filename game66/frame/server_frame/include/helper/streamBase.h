/*------------- streamBase.h
* Copyright (C): 2011
* Author: toney
* Version: 1.0
* Date: 2011/8/19 9:54:00
*
*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __STREAM_BASE_H__
#define __STREAM_BASE_H__

#include <iostream>
#include <memory.h>
#include <vector>
#include "utility/basicTypes.h"

/*************************************************************/

using namespace std;

#pragma pack(push,1)
class CStreamBase
{
public:
	virtual~CStreamBase(){}

public:
	virtual uint32	getStreamSize	()						= 0;
	virtual uint32	getPosition		()						= 0;
	virtual bool	setPosition		(uint32 newPosition)	= 0;

protected:
	virtual bool	_read			(uint32 uBytes,void* outBuffer)		= 0;
	virtual bool	_write			(uint32 uBytes,const void*inBuffer)	= 0;
public:
	virtual uint32	fprintf			(const char* pszFormat,...);

public:
	inline uint32	getSpareSize	()									{	return getStreamSize() - getPosition();					}

public:
	inline bool		read			(uint32 uBytes,void*outBuffer)		{	return _read(uBytes,outBuffer);							}
	inline bool		write			(uint32 uBytes,const void*inBuffer)	{	return _write(uBytes,inBuffer);							}

public:
	//------------------------------ 
	template <typename T>
	inline bool		read			(T* pBuffer)						{	return (pBuffer ? read	(sizeof(T),pBuffer) : false);	}
	//------------------------------
	template <typename T>
	inline bool		read_			(T& t)								{	return read(sizeof(T),&t);								}
	//------------------------------
	template <typename T>
	inline bool		write			(const T* pBuffer)					{	return (pBuffer ? write	(sizeof(T),pBuffer) : false);	}
	//------------------------------ 
	template <typename T>
	inline bool		write_			(const T& t)						{	return write(sizeof(T),&t);								}
    //------------------------------ 
    template <typename T>
    inline bool		writeObj		(const T& obj)				        {   return obj.write( this );                               }
    //------------------------------ 
    template <typename T>
    inline bool		readObj		    (const T& obj)				        {   return obj.read( this );                                }
	//------------------------------ 
	template <typename T>
	inline bool		readVector		(vector<T>&vec)
	{
		uint32 uCount = 0;
		if(!read_(uCount) || getSpareSize() < (sizeof(T) * uCount))
			return false;

		vec.resize(uCount); 
		if(uCount && !read(sizeof(T) * uCount,&(vec[0])))
			return false;

		return true;
	}
	//------------------------------ 
	template <typename T>
	inline bool		writeVector		(vector<T>&vec)
	{
		uint32 uCount = vec.size();
		if(!write_(uint32(uCount)))
			return false;

		if(uCount && !write(sizeof(T) * uCount,&(vec[0])))
			return false;

		return true;
	}

public:
	virtual bool	isEof			()							{	return (getSpareSize() == 0);			}
	virtual bool	isEmpty			()							{	return (getStreamSize() == 0);			}

public:
	virtual bool	skipPosition	(int32 _Bytes);
	virtual bool	readString		(char* stringBuf,uint32 bufferSize);
	virtual bool	writeString		(const char *stringBuf,int32 maxLen = -1);
	virtual bool	readLine		(char* buffer,uint32 bufferSize);
	virtual bool	writeLine		(const char *buffer);
	virtual bool	writeString		(const std::string&_string);
	virtual bool	readString		(std::string&_string);
};
#pragma pack(pop)

#endif // __STREAM_BASE_H__

