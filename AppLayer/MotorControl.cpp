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
volatile float GLOBAL_TORQUE_CMD;

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

int g_set_speed;

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
		objectHandle->phaseCurrents.a = objectHandle->analog.GetPhaseCurrent(0);
		objectHandle->phaseCurrents.b = objectHandle->analog.GetPhaseCurrent(1);
		objectHandle->phaseCurrents.c = objectHandle->analog.GetPhaseCurrent(2);

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
			float angleInRadians = ((float)angle / 585.0) * 2.0 * M_PI;

			float torqueCommand = objectHandle->SpeedLoop(objectHandle->systemData.runTimeData.speed, -speedFilter);

			objectHandle->TorqueLoop(torqueCommand,
					angleInRadians,
					objectHandle->phaseCurrents);
		}

		objectHandle->CheckConfigUpdates();

		osDelay(1);
	}
}

void MotorControl::TorqueLoop(float setTorque, float angleInRadians, ABC phaseCurrents)
{
	AlphaBetaZero abzFb, abzFw;
	DQZero dqzFb, dqzFw;
	ABC abcFw;

	static DQZero filteredDQZeroFb;


	// 3Phase 120deg AC >> 2 phase 90deg AC
	abzFb = ClarkTransform(phaseCurrents);

	// 2 phase 90deg AC >> 2 phase 90deg DC
	dqzFb = ParkTransform(abzFb, angleInRadians);

	filteredDQZeroFb.d = filteredDQZeroFb.d * 0.99 + dqzFb.d * 0.01;
	filteredDQZeroFb.q = filteredDQZeroFb.q * 0.99 + dqzFb.q * 0.01;

	GLOBAL_PARK_D = filteredDQZeroFb.d;
	GLOBAL_PARK_Q = filteredDQZeroFb.q;
	GLOBAL_TORQUE_CMD = setTorque;

	dqzFw.d = dController.Calculate(filteredDQZeroFb.d, setTorque);
	dqzFw.q = qController.Calculate(filteredDQZeroFb.q, 0.0);

	abzFw = InverseParkTransform(dqzFw, angleInRadians);
	abcFw = InverseClarkeTransform(abzFw);

	motorPwm.SetPwmChannel1Duty(abcFw.a * 0.5 + 500);
	motorPwm.SetPwmChannel3Duty(abcFw.b * 0.5 + 500);
	motorPwm.SetPwmChannel2Duty(abcFw.c * 0.5 + 500);
}

float MotorControl::SpeedLoop(float setSpeed, float speedFb)
{
	return speedController.Calculate(speedFb, setSpeed);
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

DQZero MotorControl::ParkTransform(AlphaBetaZero abz, float angleInRadians)
{
	DQZero result;

	float sin_theta = std::sin(angleInRadians);
	float cos_theta = std::cos(angleInRadians);

	result.d = ((abz.alpha * cos_theta) + (abz.beta * sin_theta));
	result.q = ((abz.beta * cos_theta) - (abz.alpha * sin_theta));

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
