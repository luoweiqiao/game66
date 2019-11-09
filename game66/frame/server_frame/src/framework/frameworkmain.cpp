
#include <iostream>
#include "framework.h"
#include "framework/application.h"
#include "utility/timer_mgr.h"

using namespace svrlib ;

int FrameworkMain(int argc, char * argv[])
{
    if (argc <= 0 || argv == NULL)
    {
        throw "The input argument for FrameworkMain is illegal!";
    }

	try
	{
	    CFrameWork& frameWork = CFrameWork::Instance();
	    frameWork.InitializeEnvironment(argc, argv);
		
        frameWork.Run();
		LOG_DEBUG("application shutdown");
		CApplication::Instance().ShutDown();
		CApplication::Instance().OverShutDown();
		//
	}
	catch (char const * what)
	{
		std::cout << what << std::endl ;
		LOG_ERROR("process exit");
	}
	return 0 ;
}
