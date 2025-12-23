#include "MotorControl.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"

using namespace AppLayer;

MotorControl::MotorControl(HardwareLayer::MotorPwm& motorPwmRef)
: motorPwm(motorPwmRef)
{}

Common::ErrorType MotorControl::SetArmed(bool isArmed)
{
	armed = isArmed;
	return Common::ErrorType::OK;
}

void MotorControl::SetDcMotor(int16_t duty)
{
	motorPwm.SetPwmChannel2Duty((Common::MOTOR_PWM_MAX_CNT/2) + duty/10.0);
	motorPwm.SetPwmChannel3Duty((Common::MOTOR_PWM_MAX_CNT/2) - duty/10.0);
}
