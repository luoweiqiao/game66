/*------------- luaTinker.h
* Copyright (C): 2011
* Author: toney
* Version: 1.01
* Date: 2011/9/1 16:03:44
*
*/ 
/***************************************************************
* 
***************************************************************/
#ifndef __LUA_TINKER_H__
#define __LUA_TINKER_H__

#include <iostream>
#include <stdio.h>
extern "C" 
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
};
#include "lua_tinker.h"
/*************************************************************/
//class CLuaTinker
//{
//private:
//protected:
//	lua_State*		m_luaState;
//
//public:
//	CLuaTinker()			{	m_luaState = NULL;luaRegister();	}
//	virtual~CLuaTinker()	{	luaClose();								}
//
//public:
//	void	luaRegister	()																{	luaClose();m_luaState = lua_open();luaopen_base(m_luaState);}
//	void	luaClose		()																{	if(m_luaState)lua_close(m_luaState);								}
//
//	//装载文件
//public:
//	void	dofile			(const char *filename)								{	return lua_tinker::dofile(m_luaState,filename);					}
//
//	//函数相关
//public:
//	template<typename F> 
//	void	defFunc			(const char* name,F func)							{	return lua_tinker::def(m_luaState,name,func);					}
//
//	template<typename R>
//	R		call				(const char* name)									{	return lua_tinker::call<R>(m_luaState,name);						}
//
//	template<typename R,typename T1>
//	R		call				(const char* name,T1 arg)							{	return lua_tinker::call<R>(m_luaState,name,arg);				}
//
//	template<typename R,typename T1,typename T2>
//	R		call				(const char* name,T1 arg1,T2 arg2)				{	return lua_tinker::call<R>(m_luaState,name,arg1,arg2);		}
//
//	template<typename R,typename T1,typename T2,typename T3>
//	R		call				(const char* name,T1 arg1,T2 arg2,T3 arg3)	{	return lua_tinker::call<R>(m_luaState,name,arg1,arg2,arg3);	}
//
//	//变量相关
//public:
//	template<typename T> 
//	void	setValue			(const char* name,T object)						{	return lua_tinker::set<T>(m_luaState,name,object);				}
//	template<typename T>
//	T		getValue			(const char* name)									{	return lua_tinker::get<T>(m_luaState,name);						}
//
//	//类结构相关
//public:
//	template<typename T>
//	void	defClass			(const char* name)									{	return lua_tinker::class_add<T>(m_luaState,name);				}
//	template<typename T, typename F>
//	void	defClassFunc	(const char* name,F func)							{	return lua_tinker::class_def<T>(m_luaState,name,func);		}
//	template<typename T, typename BASE, typename VAR> 
//	void	defClassValue	(const char* name,VAR BASE::*val)				{	return lua_tinker::class_mem<T>(m_luaState,name,val);			}
//	template<typename T, typename CONSTRUCTOR>
//	void	defClassCon		(CONSTRUCTOR con)										{	return lua_tinker::class_con<T>(m_luaState,con);				}
//	template<typename T, typename P>
//	void	defClassInh		()															{	return lua_tinker::class_inh<T,P>(m_luaState);					}
//};

#endif // __LUA_TINKER_H__

