/*----------------- basicFunctions.cpp
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/8/27 19:11:24
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#include "utility/basicFunctions.h"
#include <assert.h>
/*************************************************************/
//-------------------------------------------------------------
//------------------------------ 
int32	dAtoi		(const char*pString)
{
	return static_cast<int32>(dAtoll(pString));
}
//-------------------------------------------------------------
//------------------------------ 
uint32	dAtoui			(const char*pString)
{
	if(!pString)
		return 0;
	return static_cast<uint32>(strtoul(pString,NULL,0));
}
//-------------------------------------------------------------
//------------------------------ 
long	dAtol		(const char*pString)
{
	return static_cast<long>(dAtoll(pString));
}
//-------------------------------------------------------------
//------------------------------ 
double	dAtof		(const char*pString)
{
	if(!pString)
		return 0.0f;

	return atof(pString);
}
//-------------------------------------------------------------
//------------------------------ 
int64	dAtoll		(const char*pString)
{
	if(!pString)
		return 0;

#ifdef WIN32
	return _atoi64(pString);
#else//WIN32
	return atoll(pString);
#endif//WIN32
}
//-------------------------------------------------------------
//------------------------------ 
uint64	dAtoull	(const char*pString)
{
	if(!pString)
		return 0;

#ifdef WIN32
	return _strtoui64(pString,NULL,10);
#else//WIN32
	return strtoull(pString,NULL,10);
#endif//WIN32
}

/*------------------------------------------------------------------------------
**获得uint32数据中包含多少位
*/
uint8	getNumberBit(uint32 uNumber)
{
	

	uint8	uBit = 0;
	while (uNumber != 0)
	{
		uBit++;
		uNumber /= 10;
	}

	return uBit;
}
//------------------------------------------------------
//------------------------------ 获得累加数  positive
uint32	getAddValue(uint32 uFrom,uint32 uValue,uint32 nMax)
{
	

	uint32 uSum = uFrom + uValue;
	/*溢出||过大*/ 
	if (uFrom > uSum || uSum >= nMax)
		return nMax;

	return uSum;
}
//------------------------------------------------------
//------------------------------ 增加数
uint32	addValue(uint32 uValue,int nValue,uint32 nMax)
{
	

	if (nValue < 0 && uValue < uint32(-1 * nValue))
		uValue = 0;
	else
		uValue += nValue;

	if (uValue > nMax)
		uValue = nMax;

	return uValue;
}
//-------------------------------------------------------------
//------------------------------ 设置int值
bool		setInt		(int32&iOld,int32 iNew,int32 iMax,int32 iMin)
{
	
	if(iMax < iMin)
		iMax = iMin;

	if(iOld == iNew)
		return false;

	iOld = iNew;
	if(iOld < iMin)
		iOld = iMin;

	if(iOld > iMax)
		iOld = iMax;

	return true;
}
//-------------------------------------------------------------
//------------------------------ 更新int值
bool		updateInt	(int32&iValue,int32 iUpdate,int32 iMax,int32 iMin)
{
	
	if(iMax < iMin)
		iMax = iMin;

	if(iUpdate == 0)
		return false;

	if(iValue < 0 && iUpdate < 0)
	{
		iValue +=iUpdate;
		if(iValue > 0)
			iValue = iMin;
	}
	else if(iValue > 0 && iUpdate > 0)
	{
		iValue +=iUpdate;
		if(iValue < 0)
			iValue = iMax;
	}
	else
	{
		iValue +=iUpdate;
	}
	if(iValue > iMax)
		iValue = iMax;

	if(iValue < iMin)
		iValue = iMin;

	return true;
}
//-------------------------------------------------------------
//------------------------------ 能否更新int32值
bool	canUpdateUint32	(uint32 uValue,int32 iUpdate,uint32 uMax)
{
	
	if(iUpdate == 0)
		return true;

	if(iUpdate < 0)
	{
		if(uValue < uint32(iUpdate * -1))
			return false;
	}
	else
	{
		uint32 uTemp = uValue + iUpdate;
		if(uTemp > uMax || uTemp < uValue || uTemp < uint32(iUpdate))
			return false;
	}

	return true;
}
//-------------------------------------------------------------
//------------------------------ 更新int32值
bool	updateUint32	(uint32&uValue,int32 iUpdate,uint32 uMax)
{
	
	if(iUpdate < 0)
	{
		if(uValue < uint32(iUpdate * -1))
			uValue = 0;
		else
			uValue += iUpdate;
	}
	else
	{
		uint32 uTemp = uValue + iUpdate;
		if(uTemp > uMax || uTemp < uValue || uTemp < uint32(iUpdate))
			uValue = uMax;
		else
			uValue = uTemp;
	}

	return true;
}

//-------------------------------------------------------------
//------------------------------ 
bool	canUpdateUint32	(uint32 uValue,uint32 uUpdate,bool bAdd,uint32 uMax)
{
	
	if(uUpdate == 0)
		return true;

	if(bAdd)
	{
		uint32 uTemp = uValue + uUpdate;
		if(uTemp > uMax || uTemp < uValue || uTemp < uint32(uUpdate))
			return false;
	}
	else
	{
		if(uValue < uUpdate)
			return false;	
	}

	return true;
}
//-------------------------------------------------------------
//------------------------------ 
bool	updateUint32	(uint32&uValue,uint32&uUpdate,bool bAdd,uint32 uMax)
{
	
	if(bAdd)
	{
		uint32 uTemp = uValue + uUpdate;
		if(uTemp > uMax || uTemp < uValue || uTemp < uUpdate)
			uTemp = uMax;

		uUpdate -= (uTemp - uValue);
		uValue	= uTemp;
	}
	else
	{
		uint32 uTemp = 0;
		if(uValue >= uUpdate)
			uTemp = uValue - uUpdate;

		uUpdate -=(uValue - uTemp);
		uValue	= uTemp;
	}

	return true;
}
//-------------------------------------------------------------
//------------------------------ 
bool	updateUint32_	(uint32&uValue,uint32 uUpdate,bool bAdd,uint32 uMax)
{
	
	if(bAdd)
	{
		uint32 uTemp = uValue + uUpdate;
		if(uTemp > uMax || uTemp < uValue || uTemp < uUpdate)
			uTemp = uMax;

		uUpdate -= (uTemp - uValue);
		uValue	= uTemp;
	}
	else
	{
		uint32 uTemp = 0;
		if(uValue >= uUpdate)
			uTemp = uValue - uUpdate;

		uUpdate -=(uValue - uTemp);
		uValue	= uTemp;
	}

	return true;
}
//-------------------------------------------------------------
//------------------------------ 获取比例值
uint32	getUint32Proportion	(uint32 uValue,float32 fRatio,bool bRoundedUp)
{
	float32 fReturn = uValue * fRatio;
	uint32	uReturn = uint32(fReturn);
	if(bRoundedUp)
	{
		if(float32(uReturn) < fReturn)
			uReturn++;
	}
	return uReturn;
}

//-------------------------------------------------------------
//------------------------------ 除法
uint32	dDivisionUint32	(uint64 _molecular,uint32 _denominator,bool _floor)
{
	if(!_molecular || !_denominator)
		return 0;

	uint32 _return = uint32(_molecular / _denominator);
	if(!_floor && (_molecular % _denominator) > 0)
		++_return;

	return _return;
}

// 设置数组按位标志
bool  SetBitFlag(uint32* 	 szFlag,int32 len,int32 pos)
{
	if(pos >= (len*32)){
		assert(false);
		return false;
	}
	int iIdx = pos/32;
	uint32 bFlag = 0x01 << (31-(pos%32));
	szFlag[iIdx] |= bFlag;

	return true;
}
bool  UnsetBitFlag(uint32* szFlag,int32 len,int32 pos)
{
	if(pos >= (len*32)){
		assert(false);
		return false;
	}
	int iIdx = pos/32;
	uint32 bFlag = 0x01 << (31-(pos%32));
	szFlag[iIdx] &= ~bFlag;
	
	return true;
}
int  IsSetBitFlag(uint32*  szFlag,int32 len,int32 pos)
{
	if(pos >= (len*32)){
		//assert(false); 这里会导致宕机，注释掉， delete by har
		return 0;
	}
	int iIdx = pos/32;
	uint32 bFlag = 0x01 << (31-(pos%32));

	return (szFlag[iIdx] & bFlag);
}
bool  ClearBitFlag(uint32*  szFlag,int32 len)
{
	memset(szFlag,0,len*4);
	return true;
}


