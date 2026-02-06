#ifndef QUADRATIC_ENCODER_DRIVER
#define QUADRATIC_ENCODER_DRIVER

#include "IEncoder.hpp"

namespace HardwareLayer
{
	class QuadraticEncoderDriver
	: public IEncoder
	{
	public:
		QuadraticEncoderDriver();
		void Init();
		void Reset() override;
		int GetPosition() override;
		int GetSpeed() override;
		void RegisterOnIndexPulseCallback(Callback* callback) override;
	private:
	};
}

#endif //QUADRATIC_ENCODER_DRIVER
