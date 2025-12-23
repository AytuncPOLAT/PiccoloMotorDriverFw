#ifndef MOTOR_PWM_HPP
#define MOTOR_PWM_HPP

#include "stm32h7xx_hal.h"

namespace HardwareLayer
{
	class MotorPwm
	{
	public:
		MotorPwm(TIM_HandleTypeDef& timerRef);
		void PwmIoInit();
		void SetPwmChannel1Duty(uint32_t duty);
		void SetPwmChannel2Duty(uint32_t duty);
		void SetPwmChannel3Duty(uint32_t duty);
		void SetPwmChannel4Duty(uint32_t duty);

	private:
		TIM_HandleTypeDef& timerHandle;
	};

}

#endif //MOTOR_PWM_HPP
