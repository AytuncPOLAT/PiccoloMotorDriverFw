#include "MotorControl.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"
#include "PidControl.hpp"
#include "AdcDriver.hpp"
#include "math.h"

float GLOBAL_ADC_6;
float GLOBAL_ADC_0;

uint16_t G_ADC_ext0;
uint16_t G_ADC_ext1;

using namespace AppLayer;

volatile float GLOBAL_PARK_D;
volatile float GLOBAL_PARK_Q;
volatile float GLOBAL_PARK_ZERO;

DQZero dqz = {1.0, 0.0, 0.0};

volatile ABC GLOBAL_Abc;

volatile int GLOBAL_encoder = 0;

extern int Global_as5047;

volatile uint8_t globalMarker = 0;

float dFilter = 0.0;
float qFilter = 0.0;

int speed = 0;
int16_t oldPos = 0;
int16_t signedPos = 0;
float speedFilter = 0.0;
volatile float speedCmd = 0.0;

void MotorControl::OnIndexPulseCallBack()
{
}

MotorControl::MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
						   PidController& pidControllerRef,
						   AnalogProcessor& analogRef,
						   Common::SystemData& systemDataRef,
						   HardwareLayer::IEncoder& rotorEncoderRef)
: motorPwm(motorPwmRef)
, pidController(pidControllerRef)
, analog(analogRef)
, systemData(systemDataRef)
, rotorEncoder(rotorEncoderRef)
{
	dController.SetParameters(systemData.configurationData.dqController.kp / 1000.0,
			systemData.configurationData.dqController.ki / 1000.0,
			systemData.configurationData.dqController.kd / 1000.0,
			systemData.configurationData.dqController.maxIWindUp / 1000.0,
			systemData.configurationData.dqController.saturation / 1000.0);

	qController.SetParameters(systemData.configurationData.dqController.kp / 1000.0,
			systemData.configurationData.dqController.ki / 1000.0,
			systemData.configurationData.dqController.kd / 1000.0,
			systemData.configurationData.dqController.maxIWindUp / 1000.0,
			systemData.configurationData.dqController.saturation / 1000.0);

	speedController.SetParameters(systemData.configurationData.speedController.kp / 1000.0,
			systemData.configurationData.speedController.ki / 1000.0,
			systemData.configurationData.speedController.kd / 1000.0,
			systemData.configurationData.speedController.maxIWindUp / 1000.0,
			systemData.configurationData.speedController.saturation / 1000.0);
}

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
		GLOBAL_ADC_6 = objectHandle->analog.GetBusVoltage();
		G_ADC_ext0 = objectHandle->analog.GetExtAnalog(0);
		G_ADC_ext1 = objectHandle->analog.GetExtAnalog(1);

		if(objectHandle->mode == Common::MotorMode::DC_AB)
		{

		}



		if(objectHandle->analog.IsCalibrated())
		{
			Global_as5047 = objectHandle->rotorEncoder.GetPosition();

			signedPos = (int16_t)(Global_as5047 << 2);

			if(signedPos < 0)
				speed = abs(signedPos) - abs(oldPos);
			else
				speed = abs(oldPos) - abs(signedPos);

			oldPos = signedPos;

			speedFilter = speed*0.1 + speedFilter*0.9;



			angle = (Global_as5047 >> 2) + G_ADC_ext0;

			angle = angle % 585;
			float angleInRad = ((float)angle / 585.0) * 2.0 * M_PI;

			//Feed FW
			AlphaBetaZero abz;

			abz = objectHandle->InverseParkTransform(dqz, angleInRad);
			ABC phaseDutyABC = objectHandle->InverseClarkeTransform(abz);

			float power = 0.5;//objectHandle->systemData.runTimeData.pwm;

			objectHandle->motorPwm.SetPwmChannel1Duty((phaseDutyABC.a * power) + 500);
			objectHandle->motorPwm.SetPwmChannel3Duty((phaseDutyABC.b * power) + 500);
			objectHandle->motorPwm.SetPwmChannel2Duty((phaseDutyABC.c * power) + 500);
			//Feed FW

			//FeedBack
			ABC currentFeedBackABC = {objectHandle->analog.GetPhaseCurrent(0),
					objectHandle->analog.GetPhaseCurrent(1),
					objectHandle->analog.GetPhaseCurrent(2)};
			objectHandle->clarkTransformResult = objectHandle->ClarkTransform(currentFeedBackABC);
			objectHandle->parkTransformResult = objectHandle->ParkTransform(objectHandle->clarkTransformResult.alpha, objectHandle->clarkTransformResult.beta, angleInRad);

			dFilter = objectHandle->parkTransformResult.d * 0.01 + dFilter*0.99;
			qFilter = objectHandle->parkTransformResult.q * 0.01 + qFilter*0.99;

			GLOBAL_PARK_D = dFilter;
			GLOBAL_PARK_Q = qFilter;

			speedCmd = objectHandle->speedController.Calculate(-speedFilter, (G_ADC_ext1 - 2048) / 100.0);
			float dTarget = speedCmd;//objectHandle->systemData.runTimeData.syncPwm / 1000.0;

			dqz.d = objectHandle->dController.Calculate(GLOBAL_PARK_D, dTarget);
			dqz.q = objectHandle->qController.Calculate(GLOBAL_PARK_Q, 0.0);
			//FeedBack
		}

		objectHandle->CheckConfigUpdates();

		osDelay(1);
	}
}

void MotorControl::CheckConfigUpdates()
{
	if(systemData.runTimeData.isConfigChanged == true)
	{
		systemData.runTimeData.isConfigChanged = false;

		dController.SetParameters(systemData.configurationData.dqController.kp / 1000.0,
				systemData.configurationData.dqController.ki / 1000.0,
				systemData.configurationData.dqController.kd / 1000.0,
				systemData.configurationData.dqController.maxIWindUp / 1000.0,
				systemData.configurationData.dqController.saturation / 1000.0);

		qController.SetParameters(systemData.configurationData.dqController.kp / 1000.0,
				systemData.configurationData.dqController.ki / 1000.0,
				systemData.configurationData.dqController.kd / 1000.0,
				systemData.configurationData.dqController.maxIWindUp / 1000.0,
				systemData.configurationData.dqController.saturation / 1000.0);

		speedController.SetParameters(systemData.configurationData.speedController.kp / 1000.0,
				systemData.configurationData.speedController.ki / 1000.0,
				systemData.configurationData.speedController.kd / 1000.0,
				systemData.configurationData.speedController.maxIWindUp / 1000.0,
				systemData.configurationData.speedController.saturation / 1000.0);
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
