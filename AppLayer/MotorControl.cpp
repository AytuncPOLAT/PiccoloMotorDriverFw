#include "MotorControl.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"
#include "PidControl.hpp"
#include "AdcDriver.hpp"

using namespace AppLayer;

uint16_t GLOBAL_ADC_0;
uint16_t GLOBAL_ADC_1;
uint16_t GLOBAL_ADC_2;
uint16_t GLOBAL_ADC_3;
uint16_t GLOBAL_ADC_4;
uint16_t GLOBAL_ADC_5;
uint16_t GLOBAL_ADC_6;


MotorControl::MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
						   PidController& pidControllerRef,
						   HardwareLayer::AdcDriver& adcRef)
: motorPwm(motorPwmRef)
, pidController(pidControllerRef)
, adc(adcRef)
{}

void MotorControl::Init()
{
	taskHandle = xTaskCreate(this->MotorControlTask, "MotorControlTask", 128 * 4, (void*) this,
			24, NULL);
}

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

void MotorControl::SetElectricalAngle(int16_t amplitude, int16_t angle)
{
	auto phase = sinPwm.Update3P(amplitude, angle);
	motorPwm.SetPwmChannel1Duty(phase.a);
	motorPwm.SetPwmChannel2Duty(phase.b);
	motorPwm.SetPwmChannel3Duty(phase.c);
}

void MotorControl::SetMotorCurrent(int32_t current)
{}

void MotorControl::MotorControlTask(void *argument)
{
	MotorControl *objectHandle = static_cast<MotorControl*>(argument);

	uint16_t angle = 0;

	while (1)
	{
		GLOBAL_ADC_0 = objectHandle->adc.ReadChannel(0);
		GLOBAL_ADC_1 = objectHandle->adc.ReadChannel(1);
		GLOBAL_ADC_2 = objectHandle->adc.ReadChannel(2);
		GLOBAL_ADC_3 = objectHandle->adc.ReadChannel(3);
		GLOBAL_ADC_4 = objectHandle->adc.ReadChannel(4);
		GLOBAL_ADC_5 = objectHandle->adc.ReadChannel(5);
		GLOBAL_ADC_6 = objectHandle->adc.ReadChannel(6);

		if(1)//objectHandle->mode == Common::MotorMode::DC_AB)
		{
			if(angle < 4095)
			{
				angle++;
			}
			else
				angle = 0;


			objectHandle->SetElectricalAngle(400, angle);
		}

		osDelay(10);
	}
}
