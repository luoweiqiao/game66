/*----------------- rect.h
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/9/22 14:33:44
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#ifndef __RECT_H__
#define __RECT_H__

#include "utility/basicTypes.h"
#include <memory.h>
#include <math.h>
/*************************************************************/
#pragma pack(push,1)
//##############################################################################
//-------------------------------------------------------------
//------------------------------ 区域16位
struct _stRectI16
{
public:
	int16	pointX;
	int16	pointY;
	int16 extentX;
	int16 extentY;

public:
	_stRectI16&getRectI16()					{	return *this;								}

public:
	void	zero	()						{	pointX = pointY = 0;extentX = extentY = 0;	}
	void	setPoint(int16 xy)				{	pointX = pointY = xy;						}
	void	setPoint(int16 _x,int16 _y)		{	pointX = _x;pointY = _y;					}
	void	setExtent(int16 xy)				{	extentX = extentY = xy;						}
	void	setExtent(int16 _x,int16 _y)	{	extentX = _x;extentY = _y;					}
};
//##############################################################################
#pragma pack(pop)

#endif // __RECT_H__

