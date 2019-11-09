/*----------------- stl_hash_map.h
*
* Copyright (C) 2013 toney
* Author: toney
* Version: 1.0
* Date: 2012/2/24 15:58:14
*--------------------------------------------------------------
*stl hash_map封装 key为hash计算，会分配桶存储,插入速度一般比map快
-遍历速度量大量与map差不多
*------------------------------------------------------------*/
#ifndef STL_HASH_MAP_H__
#define STL_HASH_MAP_H__

#ifdef WIN32 ///windows
#include <hash_map>
#else	///linux
#include <ext/hash_map>
#endif//WIN32
#include "utility/basicTypes.h"

/*************************************************************/
template<class _Key,class _Tp>
class stl_hash_map	:
#ifdef WIN32//windows
	public stdext::hash_map<_Key,_Tp>
#else//linux
	public __gnu_cxx::hash_map<_Key,_Tp>
#endif//WIN32
{
public:
#ifdef WIN32//windows
	typedef stdext::hash_map<_Key,_Tp>				Parent;
	typedef std::pair<_Key,_Tp>						Pair;
#else//linux
	typedef __gnu_cxx::hash_map<_Key,_Tp>			Parent;
	typedef std::pair<typename __gnu_cxx::hash_map
						<_Key,_Tp>::iterator,bool>	_Pairib;
	typedef std::pair<_Key,_Tp>						Pair;
#endif//WIN32

public:
	inline typename Parent::iterator	erase_(typename Parent::iterator __it)
	{
#ifdef WIN32//windows
		return erase(__it);
#else//linux
		typename Parent::iterator erase_it = __it;
		++__it;
		Parent::erase(erase_it);
#endif//WIN32
		return __it;
	}

public:
	//--- 插入数据
	inline _Pairib insert_(const _Key& _key,const _Tp&_tp)
	{
		return Parent::insert(Pair(_key,_tp));

	}
	//--- hash_map查询地址
	inline _Tp* find_	(const _Key& _key)
	{
		typename Parent::iterator _pos = Parent::find(_key);

		return ((_pos != Parent::end()) ? &(_pos->second) : NULL);
	}

	//--- 是否包含_B
	inline bool		is_contain	(Parent&_b)
	{
		typename Parent::iterator _end_= Parent::end();
		typename Parent::iterator _pos = _b.begin();
		typename Parent::iterator _end = _b.end();
		for (;_pos != _end;++_pos)
		{
			if(Parent::find(_pos->first) == _end_)
				return false;
		}
		return true;
	}

	//--- 把B中所有数据追加到容器中
	inline void		append	(Parent&_b)
	{
		typename Parent::iterator _pos = _b.begin();
		typename Parent::iterator _end = _b.end();
		for (;_pos != _end;++_pos)
			Parent::insert(Pair(_pos->first,_pos->second));
	}
};

//#############################################################
//############################## linux下hash计算
//#############################################################
#ifndef WIN32
namespace __gnu_cxx  
{  
	template<>
	struct hash<int64>  
	{  
		size_t operator()(const int64& key) const  
		{  
			return (key >> 32) ^ key;
		}
	};

	template<>
	struct hash<uint64>  
	{  
		size_t operator()(const uint64& key) const  
		{  
			return (key >> 32) ^ key;
		}
	};

	template<class T>
	struct hash<T*>  
	{  
		size_t operator()(const T* key) const  
		{  
			return (size_t)key;
		}
	};

	template<>
	struct hash<std::string>  
	{  
		size_t operator()(const std::string& key) const  
		{  
			return __stl_hash_string(key.c_str());
		}
	};
}
#endif//WIN32

#endif // STL_HASH_MAP_H__



