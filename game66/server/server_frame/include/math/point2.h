/*----------------- point2.h
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/9/6 21:34:01
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#ifndef __POINT2_H__
#define __POINT2_H__

#include "utility/basicTypes.h"
#include <memory.h>
#include <math.h>
/*************************************************************/
#pragma pack(push,1)
//##############################################################################
struct _stPoint2F;
struct _stPoint2I16;
struct _stPoint2I
{
public:
	int32	x;
	int32	y;

public:
	_stPoint2I&getPoint2I()									{	return *this;					}

public:
	//由于有联合使用所以不能有构造函数
	_stPoint2I()											{	zero();							}
	explicit _stPoint2I(int32 xy)							{	set(xy,xy);						}
	_stPoint2I(int32 _x, int32 _y)							{	set(_x,_y);						}
	_stPoint2I(const _stPoint2I&_copy)						{	set(_copy.x,_copy.y);			}
	inline _stPoint2I(const _stPoint2F&_copy);

public:
	float32	len		()const									{	return sqrtf(float32(x*x+y*y));	}
	void	zero	()										{	x = y = 0;						}
	bool	isZero	()										{	return (x == 0 && y == 0);		}
	void	set		(int32 xy)								{	x = y = xy;						}
	void	set		(const _stPoint2I&copy)					{	set(copy.x,copy.y);				}
	void	set		(int32 _x,int32 _y)						{	x=_x;y=_y;						}

	void	setMin	(const _stPoint2I&_test)				{	x = (_test.x < x) ? _test.x : x;	y = (_test.y < y) ? _test.y : y;	}
	void	setMax	(const _stPoint2I&_test)				{	x = (_test.x > x) ? _test.x : x;	y = (_test.y > y) ? _test.y : y;	}

public:
	void	normalize(int nUint)
	{
		float32 squared = float32(x*x + y*y);
		if (squared != 0.0f)
		{
			float32 factor = float32(nUint);
			factor /= sqrtf(squared);
			x = int32(x * factor);
			y = int32(y * factor);
		}
		else
		{
			zero();
		}
	}
	//--- 填写为整数
	void	fillInteger(int32 _x,int32 _y)
	{
		if(!_x || !_y)
			return;

		int32 x_	= int32(this->x) / _x + (((int32(this->x) % _x) > 0) ? 1 : 0);
		int32 y_	= int32(this->y) / _y + (((int32(this->y) % _y) > 0) ? 1 : 0);

		this->x	= x_ * _x;
		this->y	= y_ * _y;
	}

public:
	inline _stPoint2I&operator= (const _stPoint2F&_point);

public:
	//比较运算符
	inline bool	operator==(const _stPoint2I&_test) const		{	return (x == _test.x) && (y == _test.y);										}
	inline bool	operator!=(const _stPoint2I&_test) const		{	return operator==(_test) == false;												}

public:
	//算术点
	inline _stPoint2I  operator+ (const _stPoint2I&_add)const	{	_stPoint2I p;p.set(x + _add.x,y + _add.y);						return p;		}
	inline _stPoint2I  operator- (const _stPoint2I&_reduce)const{	_stPoint2I p;p.set(x - _reduce.x,y - _reduce.y);				return p;		}
	inline _stPoint2I& operator+=(const _stPoint2I&_add)		{	x += _add.x;	y += _add.y;									return *this;	}
	inline _stPoint2I& operator-=(const _stPoint2I&_reduce)		{	x -= _reduce.x;y -= _reduce.y;									return *this;	}

public:
	//算术标量
	inline _stPoint2I  operator* (int32 _mul) const				{	_stPoint2I p;p.set(x * _mul,y * _mul);							return p;		}
	inline _stPoint2I& operator*=(int32 _mul)					{	x *= _mul;	y *= _mul;											return *this;	}
	inline _stPoint2I& operator/=(int32 _div)					{	float32 inv = 1.0f/_div;x = int32(x * inv);y = int32(y * inv);	return *this;	}

	inline _stPoint2I  operator* (const _stPoint2I&_vec)const	{	_stPoint2I p;p.set(x * _vec.x, y * _vec.y);						return p;		}
	inline _stPoint2I& operator*=(const _stPoint2I&_vec)		{	x *= _vec.x;	y *= _vec.y;									return *this;	}
	inline _stPoint2I  operator/ (const _stPoint2I&_vec)const	{	_stPoint2I p;p.set(x / _vec.x, y / _vec.y);						return p;		}
	inline _stPoint2I& operator/=(const _stPoint2I&_vec)		{	x /= _vec.x;y /= _vec.y;										return *this;	}

public:
	//一元运算符
	inline _stPoint2I operator-() const							{	 _stPoint2I p;p.set(-x, -y);									return p;		}
};
//-------------------------------------------------------------
//------------------------------ 2I16点结构
struct _stPoint2I16
{
public:
	int16	x;
	int16	y;

public:
	//由于有联合使用所以不能有构造函数
	_stPoint2I16()												{	zero();							}

public:
	void	zero()												{	x = y = 0;						}
	void	set	(int16 _x,int16 _y)								{	x=_x;y=_y;						}

public:
	inline _stPoint2I16&operator= (const _stPoint2F&_point);
	inline _stPoint2I16&operator= (const _stPoint2I&_point);
};
//------------------------------------------------------
//------------------------------ 2F点结构
struct _stPoint2F
{
public:
	float32	x;
	float32	y;

public:
	_stPoint2F&getPoint2F()								{	return *this;			}

public:
	//由于有联合使用所以不能有构造函数
	_stPoint2F()										{	zero();					}
	explicit _stPoint2F(float32 xy)						{	set(xy,xy);				}
	_stPoint2F(float32 _x, float32 _y)					{	set(_x,_y);				}
	_stPoint2F(const _stPoint2F&_copy)					{	set(_copy.x,_copy.y);	}
	inline _stPoint2F(const _stPoint2I&_copy);

public:
	float32	len	()										{	return sqrtf(x*x+y*y);	}
	void	zero()										{	x = y = 0.0f;			}
	void	set	(float32 xy)							{	x = y = xy;				}
	void	set	(const _stPoint2F&copy)					{	set(copy.x,copy.y);		}
	void	set	(float32 _x,float32 _y)					{	x=_x;y=_y;				}

	void	setMin(const _stPoint2F&_test)				{	x = (_test.x < x) ? _test.x : x;	y = (_test.y < y) ? _test.y : y;	}
	void	setMax(const _stPoint2F&_test)				{	x = (_test.x > x) ? _test.x : x;	y = (_test.y > y) ? _test.y : y;	}

public:
	void	normalize(float32 fUint = 1.0f)
	{
		float32 squared = x*x + y*y;
		if (squared != 0.0f)
		{
			float32 factor = fUint / sqrtf(squared);
			x *= factor;
			y *= factor;
		}
		else
		{
			zero();
		}
	}
	//--- 填写为整数
	void	fillInteger(int32 _x,int32 _y)
	{
		if(!_x || !_y)
			return;

		int32 x_	= int32(this->x) / _x + (((int32(this->x) % _x) > 0) ? 1 : 0);
		int32 y_	= int32(this->y) / _y + (((int32(this->y) % _y) > 0) ? 1 : 0);

		this->x	= float32(x_ * _x);
		this->y	= float32(y_ * _y);
	}
public:
	inline _stPoint2F&operator= (const _stPoint2I&_point);

public:
	//比较运算符
	inline bool	operator==(const _stPoint2F&_test) const		{	return (x == _test.x) && (y == _test.y);										}
	inline bool	operator!=(const _stPoint2F&_test) const		{	return operator==(_test) == false;												}

public:
	//算术点
	inline _stPoint2F  operator+ (const _stPoint2F&_add)const	{	_stPoint2F p;p.set(x + _add.x,y + _add.y);						return p;		}
	inline _stPoint2F  operator- (const _stPoint2F&_reduce)const{	_stPoint2F p;p.set(x - _reduce.x,y - _reduce.y);				return p;		}
	inline _stPoint2F& operator+=(const _stPoint2F&_add)		{	x += _add.x;	y += _add.y;									return *this;	}
	inline _stPoint2F& operator-=(const _stPoint2F&_reduce)		{	x -= _reduce.x;y -= _reduce.y;									return *this;	}

public:
	//算术标量
	inline _stPoint2F  operator* (float32 _mul) const			{	_stPoint2F p;p.set(x * _mul,y * _mul);							return p;		}
	inline _stPoint2F  operator/ (float32 _div) const			{	float32 inv = 1.0f / _div;_stPoint2F s;s.set(x * inv,y * inv);	return s;		}
	inline _stPoint2F& operator*=(float32 _mul)					{	x *= _mul;	y *= _mul;											return *this;	}
	inline _stPoint2F& operator/=(float32 _div)					{	float32 inv = 1.0f / _div;	x *= inv;y *= inv;					return *this;	}

	inline _stPoint2F  operator* (const _stPoint2F&_vec)const	{	_stPoint2F p;p.set(x * _vec.x, y * _vec.y);						return p;		}
	inline _stPoint2F& operator*=(const _stPoint2F&_vec)		{	x *= _vec.x;	y *= _vec.y;									return *this;	}
	inline _stPoint2F  operator/ (const _stPoint2F&_vec)const	{	_stPoint2F p;p.set(x / _vec.x, y / _vec.y);						return p;		}
	inline _stPoint2F& operator/=(const _stPoint2F&_vec)		{	x /= _vec.x;y /= _vec.y;										return *this;	}

public:
	//一元运算符
	inline _stPoint2F operator-() const							{	 _stPoint2F p;p.set(-x, -y);									return p;		}
};
//-------------------------------------------------------------
//------------------------------ 
inline _stPoint2I::_stPoint2I(const _stPoint2F&_copy)				{	set(int32(_copy.x),int32(_copy.y));							}
inline _stPoint2I&_stPoint2I::operator= (const _stPoint2F&_point)	{	set(int32(_point.x),int32(_point.y));		return *this;	}
//-------------------------------------------------------------
//------------------------------ 
inline _stPoint2I16&_stPoint2I16::operator= (const _stPoint2F&_point){	set(int16(_point.x),int16(_point.y));		return *this;	}
inline _stPoint2I16&_stPoint2I16::operator= (const _stPoint2I&_point){	set(int16(_point.x),int16(_point.y));		return *this;	}
//-------------------------------------------------------------
//------------------------------ 
inline _stPoint2F::_stPoint2F(const _stPoint2I&_copy)				{	set(float32(_copy.x),float32(_copy.y));						}
inline _stPoint2F&_stPoint2F::operator= (const _stPoint2I&_point)	{	set(float32(_point.x),float32(_point.y));	return *this;	}

//##############################################################################
#pragma pack(pop)

#endif // __POINT2_H__

