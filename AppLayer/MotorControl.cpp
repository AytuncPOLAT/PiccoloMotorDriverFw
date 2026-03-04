#include "MotorControl.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"
#include "PidControl.hpp"
#include "AdcDriver.hpp"
#include "math.h"

uint16_t G_ADC_ext0;
uint16_t G_ADC_ext1;

using namespace AppLayer;

volatile float GLOBAL_PARK_D;
volatile float GLOBAL_PARK_Q;
volatile float GLOBAL_TORQUE_CMD;
volatile float GLOBAL_POSITION_CMD;
volatile float GLOBAL_BUS_VOLTAGE;
volatile float GLOBAL_ROTOR_ANGLE_RAD;
volatile float GLOBAL_ROTOR_ANGLE;
volatile int GLOBAL_ROTOR_SPEED;
volatile int GLOBAL_multiturn;


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
						   AnalogProcessor& analogRef,
						   Common::SystemData& systemDataRef,
						   HardwareLayer::IEncoder& rotorEncoderRef)
: motorPwm(motorPwmRef)
, analog(analogRef)
, systemData(systemDataRef)
, rotorEncoder(rotorEncoderRef)
{
	SetControllerParameters();
}
void MotorControl::Init()
{
	taskHandle = xTaskCreate(this->MotorControlTask, "MotorControlTask", 128 * 4, (void*) this,
			24, NULL);

	rotorEncoder.SetRotorEncoderOffset(systemData.configurationData.motor.motorEncoderOffset);
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

void MotorControl::SetMotorCurrent(int32_t current)
{}

void MotorControl::MotorControlTask(void *argument)
{
	MotorControl *objectHandle = static_cast<MotorControl*>(argument);

	uint16_t angle = 0;

	while (1)
	{
		if(objectHandle->analog.GetConversionDoneFlag()
				&& objectHandle->analog.IsCalibrated())
		{
			objectHandle->analog.ResetConversionDoneFlag();

			HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_8);

			objectHandle->phaseCurrents.a = objectHandle->analog.GetPhaseCurrent(0);
			objectHandle->phaseCurrents.b = objectHandle->analog.GetPhaseCurrent(1);
			objectHandle->phaseCurrents.c = objectHandle->analog.GetPhaseCurrent(2);

			GLOBAL_BUS_VOLTAGE = objectHandle->analog.GetBusVoltage();
			G_ADC_ext0 = objectHandle->analog.GetExtAnalog(0);
			G_ADC_ext1 = objectHandle->analog.GetExtAnalog(1);

			//TODO: Motor type
			//if(objectHandle->mode == Common::MotorMode::DC_AB)
			//{}

			GLOBAL_ROTOR_ANGLE = objectHandle->rotorEncoder.GetPosition();
			float angleInRadians = objectHandle->rotorEncoder.GetRotorAngleInRadians();

			GLOBAL_multiturn = objectHandle->rotorEncoder.GetMultiTurnPosition();

			GLOBAL_ROTOR_ANGLE_RAD = angleInRadians;
			GLOBAL_ROTOR_SPEED = objectHandle->rotorEncoder.GetSpeed();

			if(objectHandle->systemData.configurationData.controlMode
					>= (uint8_t) Common::CONTROLLER_TYPE::POSITION)
			{
				//Position Control
				if(objectHandle->systemData.configurationData.controlMode == 4)
				{
					GLOBAL_POSITION_CMD = G_ADC_ext0;
					objectHandle->speedCommand =
											objectHandle->positionController.Calculate(GLOBAL_multiturn, GLOBAL_POSITION_CMD);
				}
				else
				{
					static float posCmdFilter = 0.0;
					posCmdFilter = 0.01*objectHandle->systemData.runTimeData.position + posCmdFilter*0.99;
					GLOBAL_POSITION_CMD = posCmdFilter;

					objectHandle->speedCommand =
						objectHandle->positionController.Calculate(GLOBAL_multiturn, GLOBAL_POSITION_CMD);
				}
			}
			else
			{
				//Speed Control or lower
				objectHandle->speedCommand = objectHandle->systemData.runTimeData.speed;
			}

			if(objectHandle->systemData.configurationData.controlMode
					>= (uint8_t) Common::CONTROLLER_TYPE::SPEED) // Speed control
			{
				//Speed Control
				objectHandle->torqueCommand = objectHandle->SpeedLoop(objectHandle->speedCommand, GLOBAL_ROTOR_SPEED);
				//objectHandle->torqueCommand = objectHandle->SpeedLoop(G_ADC_ext0/50.0, GLOBAL_ROTOR_SPEED);
			}
			else
			{
				//Torque Control
				objectHandle->torqueCommand = objectHandle->systemData.runTimeData.torque;
			}

			if(objectHandle->systemData.configurationData.controlMode
								>= (uint8_t) Common::CONTROLLER_TYPE::TORQUE)
			{
				objectHandle->TorqueLoop(objectHandle->torqueCommand,
						angleInRadians,
						objectHandle->phaseCurrents);
			}
			else if (objectHandle->systemData.configurationData.controlMode
					>= (uint8_t) Common::CONTROLLER_TYPE::ELEC_ANGLE)
			{
				objectHandle->elecAngleCommand = objectHandle->systemData.runTimeData.elecAngle;
				objectHandle->TorqueLoop(objectHandle->torqueCommand,
										(float)(objectHandle->elecAngleCommand / 585.0) * 2.0 * (float)M_PI,
										objectHandle->phaseCurrents);
			}

			objectHandle->CheckConfigUpdates();
		}
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

		SetControllerParameters();
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

void MotorControl::SetControllerParameters()
{
	rotorEncoder.SetRotorEncoderOffset(systemData.configurationData.motor.motorEncoderOffset);

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

	positionController.SetParameters(systemData.configurationData.positionController.kp / 1000.0,
			systemData.configurationData.positionController.ki / 1000.0,
			systemData.configurationData.positionController.kd / 1000.0,
			systemData.configurationData.positionController.maxIWindUp / 1000.0,
			systemData.configurationData.positionController.saturation / 1000.0);
}
