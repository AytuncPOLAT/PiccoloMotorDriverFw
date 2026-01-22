#ifndef IENCODER_HPP
#define IENCODER_HPP

#include <stdint.h>

namespace HardwareLayer
{
	class IEncoder
	{
	public:
		virtual void Reset() = 0;
		virtual int GetPosition() = 0;

		struct Callback
		{
			virtual void OnIndexPulseCallBack() = 0;
		};
		virtual void RegisterOnIndexPulseCallback(Callback *callback) = 0;
	};
}

#endif //IENCODER
