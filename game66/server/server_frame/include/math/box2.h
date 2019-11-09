/*----------------- box2.h
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/12/26 22:10:33
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#ifndef __BOX2_H__
#define __BOX2_H__

#include "point2.h"
/*************************************************************/
#pragma pack(push,1)
//##############################################################################
struct _stBox2F
{
public:
	_stPoint2F	minExtents;
	_stPoint2F	maxExtents;

public:
	_stBox2F(const _stPoint2F&center,float32 f_r)
	{
		minExtents.set(center);
		maxExtents.set(center);
		minExtents.x -= f_r;
		minExtents.y -= f_r;
		maxExtents.x += f_r;
		maxExtents.y += f_r;
	}
	_stBox2F(const _stPoint2F&in_rMin,const _stPoint2F&in_rMax)
	{
		minExtents.setMin(in_rMax);
		maxExtents.setMax(in_rMin);
	}

public:
	//--- 是否重叠
	inline bool isOverlapped(const _stBox2F& in_rOverlap) const;
};

//-------------------------------------------------------------
//------------------------------ 
inline bool _stBox2F::isOverlapped(const _stBox2F& in_rOverlap) const
{
	if (in_rOverlap.minExtents.x > maxExtents.x ||
		in_rOverlap.minExtents.y > maxExtents.y)
		return false;
	if (in_rOverlap.maxExtents.x < minExtents.x ||
		in_rOverlap.maxExtents.y < minExtents.y)
		return false;
	return true;
}
//##############################################################################
#pragma pack(pop)

#endif // __BOX2_H__

