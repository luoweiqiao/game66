/*----------------- luaConsole.cpp
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2011/9/5 19:41:12
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#include "luatinker/luaConsole.h"

/*************************************************************/
//------------------------------------------------------
//------------------------------ 
CLuaConsole::CLuaConsole()
{	
	m_luaState		= NULL;
	luaRegister();
}
//------------------------------------------------------
//------------------------------ 
CLuaConsole::~CLuaConsole()
{	
	luaClose();
}

//------------------------------------------------------
//------------------------------ 
bool	CLuaConsole::luaRegister()
{	
	luaClose();
	m_luaState = lua_open();   // 创建Lua接口指针（借用DX的术语，本质是个堆栈指针）
	luaopen_base(m_luaState);  // 加载Lua基本库
	luaL_openlibs(m_luaState); // 加载Lua通用扩展库
	
	
	return (m_luaState != NULL);
}

//------------------------------------------------------
//------------------------------ 
void	CLuaConsole::luaClose()
{	
	if(m_luaState)
		lua_close(m_luaState);

	m_luaState = NULL;
}
//------------------------------------------------------
//------------------------------ 
void	CLuaConsole::luaDoFile(const char* pFileName)
{	
	if(m_luaState && pFileName)
		lua_tinker::dofile(m_luaState,pFileName);
}
void	CLuaConsole::luaDoString(const char* szString)
{
	if(m_luaState && szString)
		lua_tinker::dostring(m_luaState,szString);
}


