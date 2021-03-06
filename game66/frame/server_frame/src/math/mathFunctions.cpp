/*----------------- mathFunctions.cpp
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/10/22 15:43:16
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#include "math/mathFunctions.h"
#include <iostream>
#include <stdlib.h>
/*************************************************************/
//#############################################################
//##############################角度/弧度
//#############################################################
//------------------------------------------------------
//------------------------------ 获得PI内的弧度
float32	getPiRadian(float32 fRadian)
{
	/*取一个旋转的最小角度180度内*/ 
	if(fRadian > M_PI_F)
		fRadian -= M_2PI_F;
	else if(fRadian < -M_PI_F)
		fRadian += M_2PI_F;

	return fRadian;
}
//-------------------------------------------------------------
//------------------------------ 获得2PI内的弧度
float32	get2PiRadian(float32 fRadian)
{
	/*取在360度旋转*/ 
	while (fRadian < 0.0f)
		fRadian += M_2PI_F;
	while (fRadian > M_2PI_F)
		fRadian -= M_2PI_F;

	return fRadian;
}

//------------------------------------------------------
//------------------------------ 获得PI内的弧度
float32	getPiRadian	(float32 m,float32 n)
{
	/*计算x与y的角度*/ 
	return getPiRadian(atan2f(n,m));
}

//------------------------------------------------------
//------------------------------ 获得2PI内的弧度
float32	get2PiRadian	(float32 m,float32 n)
{
	/*计算x与y的角度*/ 
	return get2PiRadian(atan2f(n,m));
}
//-------------------------------------------------------------
//------------------------------ 获得角度
float32	getPiAngle	(float32 fAngle)
{
	/*取一个旋转的最小角度180度内*/ 
	if(fAngle > 180.0f)
		fAngle -= 360.0f;
	else if(fAngle < -180.0f)
		fAngle += 360.0f;

	return fAngle;
}
//-------------------------------------------------------------
//------------------------------ 获得角度
float32	get2PiAngle	(float32 fAngle)
{
	/*取在360度旋转*/ 
	while (fAngle < 0.0f)
		fAngle += 360.0f;
	while (fAngle > 360.0f)
		fAngle -= 360.0f;

	return fAngle;
}
//-------------------------------------------------------------
//------------------------------ 获得角度
float32	getPiAngle	(float32 m,float32 n)
{
	return getRadianToAngle(getPiRadian(m,n));
}
//-------------------------------------------------------------
//------------------------------ 获得角度
float32	get2PiAngle	(float32 m,float32 n)
{
	return getRadianToAngle(get2PiRadian(m,n));
}

//-------------------------------------------------------------
//------------------------------ 弧度转为角度
float32	getRadianToAngle	(float32 fRadian)
{
	return fRadian / M_PI_F * 180.0f;
}
//-------------------------------------------------------------
//------------------------------ 角度转为弧度
float32	getAngleToRadian	(float32 fAngle)
{
	return fAngle / 180.0f * M_PI_F;
}

//-------------------------------------------------------------
//------------------------------ 判断弧度是否在弧度范围内
bool	inRadianRange	(float32 f2PiRadian,float32 fRange,float32 fRadian)
{
	//1全部确认在2IP范围内
	f2PiRadian	= get2PiRadian(f2PiRadian);
	fRange		= getPiRadian(fRange);
	fRadian		= get2PiRadian(fRadian);

	fRadian		= f2PiRadian - fRadian;
	//2两个相差夹角要在PI以内
	fRadian		= getPiRadian(fRadian);
	//求绝对值
	fRadian		= ::fabs(fRadian);
	if(fRadian > fRange)
		return false;

	return true;
}
//-------------------------------------------------------------
//------------------------------ 判断角度是否在角度范围内
bool	inAngleRange	(float32 f2PiAngle,float32 fRange,float32 fAngle)
{
	//1全部确认在2IP范围内
	f2PiAngle	= get2PiAngle(f2PiAngle);
	fRange		= get2PiAngle(fRange);
	fAngle		= get2PiAngle(fAngle);

	fAngle		= f2PiAngle - fAngle;
	if(fAngle > fRange || fAngle < fRange)
		return false;

	return true;
}
//-------------------------------------------------------------
//------------------------------ 获得两点间的角度
float32 get2PiAngle	(const _stPoint2F& a,const _stPoint2F& b)
{
	return get2PiAngle(float32(a.x - b.x),float32(a.y - b.y));
}

//-------------------------------------------------------------
//------------------------------ 获得两点间的弧度
float32 get2PiRadian(const _stPoint2F& a,const _stPoint2F& b)
{
	return get2PiRadian(float32(a.x - b.x),float32(a.y - b.y));
}

//#############################################################
//############################## 点
//#############################################################
//-------------------------------------------------------------
//------------------------------ 获得两点距离
float32 getDistance(const _stPoint2F& a,const _stPoint2F& b)
{
	float32 x = a.x - b.x;
	float32 y = a.y - b.y;

	return ::sqrtf(x*x+y*y);
}
int	 FastDistance(const _stPoint2I& a,const _stPoint2I& b)
{
	int x = a.x - b.x;
	int y = a.y - b.y;

	return fastDistance2D(x,y);
}
//-------------------------------------------------------------
//------------------------------ 获得点到线的距离
float32	getPointToLineDistance(const _stPoint2F& a,const _stPoint2F& b,const _stPoint2F& c)
{
	//1 点p是否与直线上a/b重合
	float ac = getDistance(a,c);
	if(ac <= 0.0f)
		return 0.0f;

	float bc = getDistance(b,c);
	if(bc <= 0.0f)
		return 0.0f;

	float ab = getDistance(a,b);
	if(ab <= 0.0f)//a,b点重合无法得到直线,直接返回点到点距离
		return ac;

	//2 钝角计算
	if(ac * ac >= (bc * bc + ab * ab))
		return bc;

	if(bc * bc >= (ac * ac + ab * ab))
		return ac;

	float l = (ab + bc + ac) / 2.0f;//周长一半
	float s = sqrtf(l * (l - ab) * (l - bc) * (l - ac));//海伦公式求面积，也可以用矢量求

	return 2.0f * s / ab;
}

//-------------------------------------------------------------
//------------------------------ 是否在范围内
bool	inRange(const _stPoint2I& a,const _stPoint2I& b,int32 iRange)
{
	_stPoint2I stDistance = a - b;
	if(::abs(stDistance.x) > ::abs(iRange) || ::abs(stDistance.y) > ::abs(iRange))
		return false;;

	return true;
}

//---快速平方根
double FastSqrt(double d)
{
	const double tolerance = 0.00001;
	double x,nx = 0.0;
	if(d < tolerance) return 0.0;
	x = d;
	while(1)
	{
		nx = 0.5 * (x + d/x);
		if(fabs(x - nx) < tolerance)break;
		x = nx;
	}
		
	return nx;
}
int	   FastSqrtInt(int d)
{
	int x,nx = 0;
	if(d <= 0)return 0;
	x = d;
	while(1)
	{
		nx = (x+d/x) >> 1;
		if(x == nx)break;
		x = nx;
	}

	return nx;
}
//---快速距离算法
int  approx_distance2D(int dx,int dy)
{
	int min,max;
	if(dx < 0)dx = -dx;
	if(dy < 0)dy = -dy;

	if(dx < dy)
	{
		min = dx;
		max = dy;
	}
	else
	{
		min = dy;
		max = dx;
	}

	return (((max<<8) + (max<<3) - (max<<4) - (max<<1) + (min<<7) - (min<<5) + (min<<3) - (min<<1))>>8);
}
int  fastDistance2D(int dx,int dy)
{
	if(dx<0)dx = -dx;
	if(dy<0)dy = -dy;

	int mn = dx;
	if(dy < dx)mn = dy;

	return ((dx + dy) - (mn>>1) - (mn>>2) + (mn>>3));	
}

