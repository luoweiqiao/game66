/*------------- mathFunctions.h
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/5/3 17:28:24
*
*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __MATH_FUNCTIONS_H__
#define __MATH_FUNCTIONS_H__


#include "utility/basicTypes.h"
#include "point2.h"
#include <math.h>
/*************************************************************/
//#############################################################
//############################## 角度/弧度
//#############################################################
//--- 获得PI内的弧度
extern float32	getPiRadian(float32 fRadian);
//--- 获得2PI内的弧度
extern float32	get2PiRadian(float32 fRadian);
//--- 获得PI内的弧度
extern float32	getPiRadian	(float32 m,float32 n);
//--- 获得2PI内的弧度
extern float32	get2PiRadian(float32 m,float32 n);

//--- 获得角度
extern float32	getPiAngle	(float32 fAngle);
extern float32	get2PiAngle	(float32 fAngle);
//--- 获得角度
extern float32	getPiAngle	(float32 m,float32 n);
extern float32	get2PiAngle	(float32 m,float32 n);

//--- 弧度转为角度
extern float32	getRadianToAngle	(float32 fRadian);
//--- 角度转为弧度
extern float32	getAngleToRadian	(float32 fAngle);

//--- 判断弧度是否在弧度范围内
extern bool		inRadianRange	(float32 f2PiRadian,float32 fRange,float32 fRadian);
//--- 判断角度是否在角度范围内
extern bool		inAngleRange	(float32 f2PiAngle,float32 fRange,float32 fAngle);

//--- 获得两点间的角度
extern float32	get2PiAngle	(const _stPoint2F& a,const _stPoint2F& b);
//--- 获得两点间的弧度
extern float32	get2PiRadian(const _stPoint2F& a,const _stPoint2F& b);

//#############################################################
//############################## 点
//#############################################################
//------------------------------ 
//--- 获得两点距离
extern float32	getDistance(const _stPoint2F& a,const _stPoint2F& b);
extern int		FastDistance(const _stPoint2I& a,const _stPoint2I& b);

//--- 获得点到线的距离(a和b是线段的两个端点， c是检测点)
extern float32	getPointToLineDistance(const _stPoint2F& a,const _stPoint2F& b,const _stPoint2F& p);

//--- 是否在范围内
extern bool	inRange(const _stPoint2I& a,const _stPoint2I& b,int32 iRange);

//---快速平方根
extern double FastSqrt(double d);
extern int	  FastSqrtInt(int d);

//---快速距离算法
extern int  approx_distance2D(int dx,int dy);
extern int  fastDistance2D(int dx,int dy);

#endif // __MATH_FUNCTIONS_H__

