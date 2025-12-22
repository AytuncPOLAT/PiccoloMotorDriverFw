#ifndef I_CALLBACK_HPP
#define I_CALLBACK_HPP

#include <stdint.h>

namespace Common
{
	class ICallback
	{
	public:
		struct GenericCallback
		{
			virtual void OnCallback(uint8_t arg) = 0;
		};
		virtual void RegisterCallback(GenericCallback* callBack) = 0;
	};
}

#endif
