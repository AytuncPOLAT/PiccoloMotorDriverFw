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
		virtual int GetSpeed() = 0;
		virtual float GetRotorAngleInRadians(){};
		virtual void SetRotorEncoderOffset(int16_t newOffset){};

		struct Callback
		{
			virtual void OnIndexPulseCallBack() = 0;
		};
		virtual void RegisterOnIndexPulseCallback(Callback *callback) = 0;
	};
}

#endif //IENCODER
