
#ifndef __CONFIG_CHECKER_H__
#define __CONFIG_CHECKER_H__

#include <string>
#include "framework/logger.h"
#include "framework.h"
#include "fundamental/noncopyable.h"
#include "utility/singleton.h"

namespace svrlib
{
class CConfigChecker : public ITimerSink, public AutoDeleteSingleton<CConfigChecker>
{
	enum
	{
		CHECKRELOAD_INTERVAL = 3000,	
	} ;
public:
	CConfigChecker()
    :m_bReloadConfig(false)
    ,m_pTimer(NULL)
	{
	}
	virtual void OnTimer(uint8 uiID)
	{
		if(m_bReloadConfig)
		{
			try
			{
				CFrameWork::Instance().ReloadConfig();
			}
			catch (char const * what)
			{
				using namespace svrlib;
				std::cout << "reload config error, err = " << what << std::endl ;
			}
			m_bReloadConfig = false;
		}
	}

	void Start()
	{	    
        m_pTimer = CApplication::Instance().MallocTimer(this,0);
        m_pTimer->StartTimer(CHECKRELOAD_INTERVAL,CHECKRELOAD_INTERVAL);
	}
    void Stop()
    {
        if(m_pTimer){
            CApplication::Instance().FreeTimer(m_pTimer);
            m_pTimer = NULL;
        }
    }
	void ActivateReload()
	{
	    m_bReloadConfig = true;
	}

private:
	bool    m_bReloadConfig;
    CTimer* m_pTimer;
};
}

#endif // __CONFIG_CHECKER_H__

