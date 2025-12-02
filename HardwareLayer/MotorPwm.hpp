#ifndef MOTOR_PWM_HPP
#define MOTOR_PWM_HPP

#include "stm32h7xx_hal.h"

namespace HardwareLayer
{
	class MotorPwm
	{
	public:
		MotorPwm();
		void PwmIoInit();
		void SetPwmChannelDuty(uint8_t channel, uint32_t duty);
	private:
		TIM_HandleTypeDef htim1;
	};

}

#endif //MOTOR_PWM_HPP
