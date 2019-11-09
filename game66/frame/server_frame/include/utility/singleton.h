
// Singleton.h
#ifndef __SINGLETON__
#define __SINGLETON__


class  NonCopyableClass
{
private:
	NonCopyableClass( const NonCopyableClass & );
	const NonCopyableClass & operator=( const NonCopyableClass & );

public:
	NonCopyableClass(){};
	virtual ~NonCopyableClass(){};
};


// 自动释放
// 谨慎使用，不当的使用会在释放析构顺序上产生问题
template< typename TYPE, typename REFTYPE = TYPE >
class AutoDeleteSingleton : private NonCopyableClass
{
public:
	static  REFTYPE &
	Instance()
	{
		static  TYPE  s_SingleObj;
		return  s_SingleObj;
	}

protected:
	AutoDeleteSingleton(){};
	virtual ~AutoDeleteSingleton(){};
};


// 手动执行释放
template< typename TYPE, typename PTYPE = TYPE >
class ManualDeleteSingleton : private NonCopyableClass
{
public:
	static PTYPE *
	Instance()
	{
		// 不能在释放后再使用
		if( s_bDestroy )
			return ( NULL );

		if( s_pSingleObj == NULL )
		{
			// 如果在多线程中使用需要考虑同步
			if( s_pSingleObj == NULL )
				s_pSingleObj = new TYPE();
		}

		return s_pSingleObj;
	}

	static void
	Destroy()
	{
		if( s_pSingleObj != NULL )
		{
			delete (s_pSingleObj);
			s_pSingleObj = NULL;
			s_bDestroy = true;
		}
	}

protected:
	static bool      s_bDestroy;
	static PTYPE    *s_pSingleObj;

protected:
	ManualDeleteSingleton(){};
	virtual ~ManualDeleteSingleton(){};
};

template< typename TYPE, typename PTYPE> PTYPE * ManualDeleteSingleton<TYPE, PTYPE>::s_pSingleObj  = NULL;
template< typename TYPE, typename PTYPE> bool    ManualDeleteSingleton<TYPE, PTYPE>::s_bDestroy    = false;
 
#endif

