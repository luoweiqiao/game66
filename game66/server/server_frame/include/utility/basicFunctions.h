/*------------- basicFunctions.h
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/1/29 17:46:12
*
*/ 
/***************************************************************
* 基本函数定义
***************************************************************/
#ifndef __BASIC_FUNCTIONS_H__
#define __BASIC_FUNCTIONS_H__

#include "utility/basicTypes.h"

/*************************************************************/
extern int32	dAtoi			(const char*pString);
extern uint32	dAtoui			(const char*pString);
extern long		dAtol			(const char*pString);
extern double	dAtof			(const char*pString);
extern int64	dAtoll			(const char*pString);
extern uint64	dAtoull			(const char*pString);
//------------------------------------------------------
//------------------------------- Value Function...
//--- 获得uint32数据中包含多少位
extern uint8	getNumberBit	(uint32 uNumber);

//--- 获得累加数  positive
extern uint32	getAddValue		(uint32 uFrom,uint32 uValue,uint32 nMax);

//--- 增加数
extern uint32	addValue		(uint32 uValue,int nValue,uint32 nMax = uint32(-1));

//--- 设置int值
extern bool		setInt			(int32&iOld,int32 iNew,int32 iMax = 0x7FFFFFFF,int32 iMin = 0/*0x80000000*/);
//--- 更新int值
extern bool		updateInt		(int32&iValue,int32 iUpdate,int32 iMax = 0x7FFFFFFF,int32 iMin = 0);
//--- 能否更新int32值
extern bool		canUpdateUint32	(uint32 uValue,int32 iUpdate,uint32 uMax = 0xFFFFFFFF);
//--- 更新int32值
extern bool		updateUint32	(uint32&uValue,int32 iUpdate,uint32 uMax = 0xFFFFFFFF);
extern bool		canUpdateUint32	(uint32 uValue,uint32 uUpdate,bool bAdd,uint32 uMax = 0xFFFFFFFF);
extern bool		updateUint32	(uint32&uValue,uint32&uUpdate,bool bAdd,uint32 uMax = 0xFFFFFFFF);
extern bool		updateUint32_	(uint32&uValue,uint32 uUpdate,bool bAdd,uint32 uMax = 0xFFFFFFFF);
//--- 获取比例值
extern uint32	getUint32Proportion	(uint32 uValue,float32 fRatio,bool bRoundedUp = true);

/*--- 除法
@_molecular		=分子
@_denominator	=分母
@_floor			=向下取整
*/
extern uint32	dDivisionUint32	(uint64 _molecular,uint32 _denominator,bool _floor = true);

//-------------------------------------------------------------
//------------------------------ 指针排序(空指针在后)
template<class T>
inline void	_qsort_pointer(T *_list, const uint32&_count)
{
	if(!_list || !_count)
		return;

	for (uint32 i = 0;i < _count;++i)
	{
		if(_list[i])
			continue;

		bool _empty = true;
		for (uint32 j = i + 1;j < _count;++j)
		{
			if(!_list[j])
				continue;

			_empty	= false;
			_list[i]= _list[j];
			_list[j]= NULL;
			break;
		}
		if(_empty)
			break;
	}
}

//-------------------------------------------------------------
//------------------------------ 设置指针值
template<class _T>
inline void	setValue(_T*_pointer,_T _value)
{
	if(_pointer)
		*_pointer = _value;
}
// 获取数组长度
template <class T>
int getArrayLen(T& array)
{
    return (sizeof(array) / sizeof(array[0]));
}

// 设置数组按位标志
bool  SetBitFlag(uint32*   szFlag,int32 len,int32 pos);
bool  UnsetBitFlag(uint32* szFlag,int32 len,int32 pos);
int   IsSetBitFlag(uint32* szFlag,int32 len,int32 pos);
bool  ClearBitFlag(uint32* szFlag,int32 len);

#endif // __BASIC_FUNCTIONS_H__

