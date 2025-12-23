#ifndef MOTOR_CONTROL_HPP
#define MOTOR_CONTROL_HPP

#include "stdint.h"
#include "ErrorHandler.hpp"

namespace Common
{
	enum class MotorMode : uint8_t
	{
		DC_AB = 0,
	};
}

namespace HardwareLayer
{
	class MotorPwm;
}

namespace AppLayer
{
	class MotorControl
	{
	public:
		MotorControl(HardwareLayer::MotorPwm& motorPwmRef);
		void SetDcMotor(int16_t duty);
		Common::ErrorType SetArmed(bool isArmed);

	private:
		Common::MotorMode mode;
		bool armed = false;
		HardwareLayer::MotorPwm& motorPwm;
	};

}

#endif
