#ifndef TIMERBASE_H_
#define TIMERBASE_H_

#include "utility/basicTypes.h"

namespace svrlib
{
/**
 * <p>定时器</p>
 * <p>20011-11 修改</p>
 * @author  toney
 */
	struct ITimerSink
	{
		virtual void  OnTimer(uint8 eventID) = 0;
	};

}

#endif /* TIMERBASE_H_ */
