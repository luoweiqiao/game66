#ifndef _SIZE_H_2004_03_29
#define _SIZE_H_2004_03_29

#include "type.h"

namespace fund
{
	struct Nil
	{
	};

	struct NotNil
	{
	};

	namespace size
	{
		char CalcSize(Nil);
		int CalcSize(...);
	};

	template <typename T>
	struct Size 
	{
 		enum {SIZE = sizeof(size::CalcSize(*static_cast<T*>(0)))};
		typedef Int2Type<SIZE> RET;
	};
};

#endif
