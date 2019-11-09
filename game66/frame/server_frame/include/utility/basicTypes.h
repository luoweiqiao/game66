/*------------- basicTypes.h
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/1/29 15:47:59
*
*/ 
/***************************************************************
* 定义各种基本类型
***************************************************************/
#ifndef BASIC_TYPES_H__
#define BASIC_TYPES_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/*************************************************************/
//------------------------------- Basic Types...
typedef signed char			int8;	///< Compiler independent Signed Char
typedef unsigned char		uint8;	///< Compiler independent Unsigned Char

typedef signed short		int16;	///< Compiler independent Signed 16-bit short
typedef unsigned short		uint16;	///< Compiler independent Unsigned 16-bit short

typedef signed int			int32;	///< Compiler independent Signed 32-bit integer
typedef unsigned int		uint32;	///< Compiler independent Unsigned 32-bit integer

typedef unsigned long		ulong;///< Compiler independent Unsigned 32-bit integer

typedef float				float32;///< Compiler independent 32-bit float
typedef double				float64;///< Compiler independent 64-bit float

#ifdef WIN32
typedef __int64				int64;	///Compiler independent Unsigned 64-bit integer
typedef unsigned __int64	uint64;	///Compiler independent Unsigned 64-bit integer
#else//WIN32
typedef long long			int64;
typedef unsigned long long	uint64;
#endif//WIN32

typedef char*				p_str;
typedef const char*			pc_str;
//------------------------------------------------------
//typedef unsigned char		byte;
typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;

#ifndef NULL
#define NULL	0
#endif//NULL
//------------------------------------------------------
//------------------------------- min/max Operate...
//最小值
#ifndef MIN
#define MIN(a,b)	(a > b ? b: a)
#endif//MIN
//最大值
#ifndef MAX
#define MAX(a,b)	(a > b ? a: b)
#endif//MAX
//范围值
#ifndef RANGE
#define RANGE(a,mina,maxa)  (a = MAX(mina,MIN(a,maxa)))
#endif//

#ifndef SAFE_DELETE
#define SAFE_DELETE( p)				{ if ( p) { delete ( p); ( p) = NULL; } }
#endif


//------------------------------------------------------
//------------------------------- Bit Operate...
#ifndef _BIT32
#define _BIT32(x) ((x < 32) ? (1 << (x)) : 0) 					///< Returns value with bit x set (2^x)
#endif//_BIT32

#ifndef _SET_BIT32
#define _SET_BIT32(mark,bit,s) (s ? (mark |= _BIT32(bit)) : (mark &=~_BIT32(bit)))
#endif//_SET_BIT32

#ifndef _CHECK_BIT
#define _CHECK_BIT(mark,bit)	((mark & bit) > 0)				///检测是否存在相同位
#endif//_CHECK_BIT
//------------------------------------------------------
//------------------------------------------------------
//------------------------------- 
#define MAKE_UINT64(h,l)((uint64)( (((uint64)((uint32)(h) & 0xffffffff)) << 32) | ((uint32)((uint64)(l) & 0xffffffff)) ))
#define U64_H_U32(_v)	((uint32)(((uint64)(_v) >> 32) & 0xffffffff))
#define U64_L_U32(_v)	((uint32)((uint64)(_v) & 0xffffffff))

#define MAKE_UINT32(h,l)((uint32)( (((uint32)((uint16)(h) & 0xffff)) << 16) | ((uint16)((uint32)(l) & 0xffff)) ))
#define U32_H_U16(_v)	((uint16)(((uint32)(_v) >> 16) & 0xffff))
#define U32_L_U16(_v)	((uint16)((uint32)(_v) & 0xffff))
#define MAKEU16(a, b)	((WORD)(((BYTE)((DWORD)(a) & 0xff))		| ((WORD)((BYTE)((DWORD)(b) & 0xff))) << 8))
#define MAKEU32(a, b)	((LONG)(((WORD)((DWORD)(a) & 0xffff))	| ((DWORD)((WORD)((DWORD)(b) & 0xffff))) << 16))
#define LU16(l)			((WORD)((DWORD)(l) & 0xffff))
#define HU16(l)			((WORD)((DWORD)(l) >> 16))
#define LU8(w)			((BYTE)((DWORD)(w) & 0xff))
#define HU8(w)			((BYTE)((DWORD)(w) >> 8))

/*(type *)0：把0地址当成type类型的指针。
((type *)0)->field：对应域的变量。
&((type *)0)->field:取该变量的地址，其实就等于该域相对于0地址的偏移量。
(size_t)&(((type *)0)->field)：将该地址（偏移量）转化为size_t型数据。*/
#define OFFSETOF(s,m)	offsetof(s,m)			///计算数据结构成员变量偏移值(struct,成员变量)

//#############################################################
//############################## 三角形数据
//#############################################################
#ifndef M_PI
#define M_PI		3.1415926535897932384626433
#endif//M_PI

#ifndef M_2PI
#define M_2PI		(M_PI * 2.0)
#endif//M_2PI

#ifndef M_PI_F
#define M_PI_F		3.1415926535897932384626433f
#endif//M_PI_F

#ifndef M_2PI_F
#define M_2PI_F	(M_PI_F * 2.0f)
#endif//M_2PI_F

//#############################################################
//############################## 常量
//#############################################################
#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

#endif // BASIC_TYPES_H__



