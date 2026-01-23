#include "MotorControl.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"
#include "PidControl.hpp"
#include "AdcDriver.hpp"
#include "math.h"

using namespace AppLayer;

float GLOBAL_ADC_0;
float GLOBAL_ADC_1;
float GLOBAL_ADC_2;
uint16_t GLOBAL_ADC_3;
uint16_t GLOBAL_ADC_4;
uint16_t GLOBAL_ADC_5;
uint16_t GLOBAL_ADC_6;

volatile float GLOBAL_PARK_D;
volatile float GLOBAL_PARK_Q;
volatile float GLOBAL_PARK_ZERO;

volatile ABC GLOBAL_Abc;

volatile int GLOBAL_encoder = 0;

void MotorControl::OnIndexPulseCallBack()
{
	}

MotorControl::MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
						   PidController& pidControllerRef,
						   HardwareLayer::AdcDriver& adcRef,
						   Common::SystemData& systemDataRef,
						   HardwareLayer::IEncoder& rotorEncoderRef)
: motorPwm(motorPwmRef)
, pidController(pidControllerRef)
, adc(adcRef)
, systemData(systemDataRef)
, rotorEncoder(rotorEncoderRef)
{}

void MotorControl::Init()
{
	taskHandle = xTaskCreate(this->MotorControlTask, "MotorControlTask", 128 * 4, (void*) this,
			24, NULL);

	//pidController.SetParameters(syste, _ki, _kd, _pidOutputLimit, _windUpLimit)
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

void MotorControl::SetElectricalAngle(int16_t amplitude, float angle)
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
		GLOBAL_ADC_0 = ((float)objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::DC_BUS_VOLTAGE) - 40.0 - 2048.0) / 4096.0;
		GLOBAL_ADC_1 = ((float)objectHandle->adc.ReadChannel(1) - 55.0 - 2048.0) / 4096.0;
		GLOBAL_ADC_2 = ((float)objectHandle->adc.ReadChannel(2) - 56.0 - 2048.0) / 4096.0;
		GLOBAL_ADC_3 = objectHandle->adc.ReadChannel(3);
		GLOBAL_ADC_4 = objectHandle->adc.ReadChannel(4);
		GLOBAL_ADC_5 = objectHandle->adc.ReadChannel(5);

		if(objectHandle->mode == Common::MotorMode::DC_AB)
		{

		}

		if(1)
		{
			angle = objectHandle->rotorEncoder.GetPosition()%4096;

			GLOBAL_encoder = objectHandle->rotorEncoder.GetPosition();



			float angleInRad = ((float)angle / 4096.0) * 2.0 * M_PI;

			DQZero dqz = {1.0, 0.0, 0.0};

			AlphaBetaZero abz = objectHandle->InverseParkTransform(dqz, angleInRad);
			ABC abc = objectHandle->InverseClarkeTransform(abz);

			GLOBAL_Abc.a = abc.a;
			GLOBAL_Abc.b = abc.b;
			GLOBAL_Abc.c = abc.c;

			objectHandle->clarkTransformResult = objectHandle->ClarkTransform(abc);
			objectHandle->parkTransformResult = objectHandle->ParkTransform(objectHandle->clarkTransformResult.alpha, objectHandle->clarkTransformResult.beta, angleInRad);

			GLOBAL_PARK_D = objectHandle->parkTransformResult.d;
			GLOBAL_PARK_Q = objectHandle->parkTransformResult.q;
			GLOBAL_PARK_ZERO = objectHandle->parkTransformResult.zero;

			int duty = objectHandle->systemData.runTimeData.pwm;

			objectHandle->motorPwm.SetPwmChannel1Duty((abc.a + 1) * duty);
			objectHandle->motorPwm.SetPwmChannel2Duty((abc.b + 1) * duty);
			objectHandle->motorPwm.SetPwmChannel3Duty((abc.c + 1) * duty);

			//objectHandle->SetDcMotor(objectHandle->systemData.runTimeData.pwm);
			objectHandle->systemData.runTimeData.current = GLOBAL_ADC_3;
		}

		osDelay(1);
	}
}

AlphaBetaZero MotorControl::ClarkTransform(ABC input)
{
	AlphaBetaZero output;
    const float one_over_sqrt3 = 0.5773503f;

    // Alpha aligns with phase A
    output.alpha = input.a;

    // Beta is calculated from phases B and C
    output.beta = (input.b - input.c) * one_over_sqrt3;

    return output;
}

ABC MotorControl::InverseClarkeTransform(AlphaBetaZero input)
{
    ABC output;
    const float sqrt3_2 = 0.8660254f; // sqrt(3)/2

    output.a = input.alpha;
    output.b = -0.5f * input.alpha + sqrt3_2 * input.beta;
    output.c = -0.5f * input.alpha - sqrt3_2 * input.beta;

    return output;
}

DQZero MotorControl::ParkTransform(float alpha, float beta, float angleInRad)
{
	float sin_theta = std::sin(angleInRad);
	float cos_theta = std::cos(angleInRad);

	DQZero result;

	result.d = ((alpha * cos_theta) + (beta * sin_theta));
	result.q = ((beta * cos_theta) - (alpha * sin_theta));

	return result;
}

AlphaBetaZero MotorControl::InverseParkTransform(DQZero input, float theta)
{
	AlphaBetaZero output;
    float cosTheta = std::cos(theta);
    float sinTheta = std::sin(theta);

    // Standard D-axis alignment
    output.alpha = input.d * cosTheta - input.q * sinTheta;
    output.beta  = input.d * sinTheta + input.q * cosTheta;
    output.zero  = input.zero;

    return output;
}
