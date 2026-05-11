#include "MotorControl.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"
#include "PidControl.hpp"
#include "AdcDriver.hpp"
#include "SignalProcessing.hpp"
#include "math.h"

uint16_t G_ADC_ext0;
uint16_t G_ADC_ext1;

using namespace AppLayer;

TaskHandle_t gMotorControlTaskHandle = NULL;

volatile float GLOBAL_PARK_D;
volatile float GLOBAL_PARK_Q;
volatile float GLOBAL_TORQUE_CMD;
volatile float GLOBAL_POSITION_CMD;
volatile float GLOBAL_BUS_VOLTAGE;
volatile float GLOBAL_ROTOR_ANGLE_RAD;
volatile float GLOBAL_ROTOR_ANGLE;
volatile int GLOBAL_ROTOR_SPEED;
volatile int GLOBAL_multiturn;

volatile float GLOBAL_PHASE_CURRENT_A;
volatile float GLOBAL_PHASE_CURRENT_B;
volatile float GLOBAL_PHASE_CURRENT_C;

volatile uint16_t GLOBAL_PHASE_CURRENT_A_PWM;
volatile uint16_t GLOBAL_PHASE_CURRENT_B_PWM;
volatile uint16_t GLOBAL_PHASE_CURRENT_C_PWM;

volatile int16_t GLOBAL_Q_ENCODER;

float dFilter = 0.0;
float qFilter = 0.0;

int speed = 0;
int16_t oldPos = 0;
int16_t signedPos = 0;
float speedFilter = 0.0;
volatile float speedCmd = 0.0;

int g_set_speed;

static float s_k = 0.5f;

void MotorControl::OnIndexPulseCallBack()
{
}

MotorControl::MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
						   AnalogProcessor& analogRef,
						   Common::SystemData& systemDataRef,
						   HardwareLayer::IEncoder& rotorEncoderRef,
						   HardwareLayer::IEncoder& quadEncoderRef)
: motorPwm(motorPwmRef)
, analog(analogRef)
, systemData(systemDataRef)
, rotorEncoder(rotorEncoderRef)
, quadEncoder(quadEncoderRef)
, positionCommandFilter(0.01f)
, parkDFilter(0.01f)
, parkQFilter(0.01f){
	SetControllerParameters();
}
void MotorControl::Init()
{
	BaseType_t result = xTaskCreate(this->MotorControlTask, "MotorControlTask", 128 * 4, (void*) this,
			24, &taskHandle);
	if (result == pdPASS)
	{
		gMotorControlTaskHandle = taskHandle;
	}
/*
	BaseType_t encoderTaskHandle = xTaskCreate(this->EncoderUpdater, "EncoderUpdater", 128 * 4, (void*) this,
			24, NULL);
*/
	rotorEncoder.SetRotorEncoderOffset(systemData.configurationData.motor.motorEncoderOffset);
	CalculateMotorParameters();
}

void MotorControl::CalculateMotorParameters()
{
	electricalAngle.full = 4096 / systemData.configurationData.motor.motorPoles;
	electricalAngle.half = electricalAngle.full / 2;
	electricalAngle.quarter = electricalAngle.full / 4;
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

void MotorControl::EncoderUpdater(void *argument)
{
	MotorControl *objectHandle = static_cast<MotorControl*>(argument);

	while (1)
	{
		osDelay(1000);
	}
}

void MotorControl::MotorControlTask(void *argument)
{
	MotorControl *objectHandle = static_cast<MotorControl*>(argument);

	for (;;)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, (GPIO_PinState)1);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, (GPIO_PinState)0);

		if(
				//objectHandle->analog.GetConversionDoneFlag() &&
				objectHandle->analog.IsCalibrated())
		{
			//GLOBAL_Q_ENCODER = objectHandle->rotorEncoder.GetPosition_Async();

			objectHandle->phaseCurrents.a = objectHandle->analog.GetPhaseCurrent(0);
			objectHandle->phaseCurrents.b = objectHandle->analog.GetPhaseCurrent(1);
			objectHandle->phaseCurrents.c = objectHandle->analog.GetPhaseCurrent(2);

			//G_ADC_ext0 = objectHandle->analog.GetExtAnalog(0);
			//G_ADC_ext1 = objectHandle->analog.GetExtAnalog(1);



			objectHandle->angleInRadians = objectHandle->RotorAngleInCountsToElectricalAngleInRadians(objectHandle->rotorEncoder.GetPosition_Async(),
								objectHandle->systemData.configurationData.motor.motorPoles);

			objectHandle->multiturn = objectHandle->rotorEncoder.GetMultiTurnPosition();
			objectHandle->rotorSpeed = objectHandle->rotorEncoder.GetSpeed();


			objectHandle->positionCmd = 0.0f;

			//TODO: Refactor this control flow. It is very messy right now.
			if (objectHandle->systemData.configurationData.controlMode
								== 5)
			{
				objectHandle->parkValues = objectHandle->TorqueLoop(200,0,objectHandle->phaseCurrents);
				osDelay(1000);

				int angle = (objectHandle->rotorEncoder.GetPosition() >> 2) % (int)objectHandle->electricalAngle.full;

				objectHandle->systemData.configurationData.motor.motorEncoderOffset = angle + objectHandle->electricalAngle.quarter/2;
				objectHandle->systemData.configurationData.controlMode = 1;
			}
			else
			{

				if(objectHandle->systemData.configurationData.controlMode
						>= (uint8_t) Common::CONTROLLER_TYPE::POSITION)
				{
					//Position Control
					if(objectHandle->systemData.configurationData.controlMode == 4)
					{
						objectHandle->positionCmd = (float)G_ADC_ext0;
						objectHandle->speedCommand =
												objectHandle->positionController.Calculate(objectHandle->multiturn, objectHandle->positionCmd);
					}
					else
					{

						if(objectHandle->systemData.realtimeData.position > objectHandle->systemData.configurationData.motor.positionHomeMax)
							objectHandle->systemData.realtimeData.position = objectHandle->systemData.configurationData.motor.positionHomeMax;

						else if (objectHandle->systemData.realtimeData.position < objectHandle->systemData.configurationData.motor.positionHomeMin)
							objectHandle->systemData.realtimeData.position = objectHandle->systemData.configurationData.motor.positionHomeMin;

						objectHandle->positionCmd = objectHandle->positionCommandFilter.Update((float)objectHandle->systemData.realtimeData.position);

						objectHandle->speedCommand =
							objectHandle->positionController.Calculate(objectHandle->multiturn, objectHandle->positionCmd);
					}
				}
				else
				{
					//Speed Control or lower
					objectHandle->speedCommand = objectHandle->systemData.realtimeData.speed;
				}

				if(objectHandle->systemData.configurationData.controlMode
						>= (uint8_t) Common::CONTROLLER_TYPE::SPEED) // Speed control
				{
					//Speed Control
					objectHandle->torqueCommand = objectHandle->SpeedLoop(objectHandle->speedCommand, GLOBAL_ROTOR_SPEED);
				}
				else
				{
					//Torque Control
					objectHandle->torqueCommand = objectHandle->systemData.realtimeData.torque;
				}

				if(objectHandle->systemData.configurationData.controlMode
									>= (uint8_t) Common::CONTROLLER_TYPE::TORQUE)
				{


					objectHandle->parkValues = objectHandle->TorqueLoop(objectHandle->torqueCommand,
							objectHandle->angleInRadians,
							objectHandle->phaseCurrents);
				}
				else if (objectHandle->systemData.configurationData.controlMode
						>= (uint8_t) Common::CONTROLLER_TYPE::ELEC_ANGLE)
				{
					objectHandle->elecAngleCommand = objectHandle->systemData.realtimeData.elecAngle;
					objectHandle->parkValues = objectHandle->TorqueLoop(objectHandle->torqueCommand,
											(float)(objectHandle->elecAngleCommand / 585.0) * 2.0 * (float)M_PI,
											objectHandle->phaseCurrents);
				}

				if(objectHandle->systemData.configurationData.controlMode
						> (uint8_t) Common::CONTROLLER_TYPE::ELEC_ANGLE)
				{
					objectHandle->rotorEncoder.StartAsyncRead();
				}

			}

			objectHandle->systemData.realtimeData.multiTurnEncoder = objectHandle->multiturn;

			objectHandle->systemData.realtimeData.speedGet = objectHandle->rotorSpeed;
			objectHandle->systemData.realtimeData.torqueGet = objectHandle->telemetryMotorCurrent;

			objectHandle->DebugMonitor();
			objectHandle->CheckConfigUpdates();


		}
	}
}

DQZero MotorControl::TorqueLoop(float setTorque, float angleInRadians, ABC phaseCurrents)
{
	AlphaBetaZero abzFb, abzFw;
	DQZero dqzFb, dqzFw;
	ABC abcFw;

	// 3Phase 120deg AC >> 2 phase 90deg AC
	abzFb = ClarkTransform(phaseCurrents);

	// 2 phase 90deg AC >> 2 phase 90deg DC
	dqzFb = ParkTransform(abzFb, angleInRadians);

	dqzFw.d = dController.Calculate(dqzFb.d, setTorque);
	dqzFw.q = qController.Calculate(dqzFb.q, 0.0);

	abzFw = InverseParkTransform(dqzFw, angleInRadians);
	abcFw = InverseClarkeTransform(abzFw);

	UpdateSVPWM(abzFw.alpha / 100.0, abzFw.beta / 100.0);

	telemetryMotorCurrent = static_cast<int> (dqzFb.d * 10);

	return dqzFb; // return the filtered park values
}

float MotorControl::SpeedLoop(float setSpeed, float speedFb)
{
	return speedController.Calculate(speedFb, setSpeed);
}

void MotorControl::DebugMonitor()
{
	GLOBAL_PARK_D = parkValues.d;
	GLOBAL_PARK_Q = parkValues.q;
	GLOBAL_TORQUE_CMD = torqueCommand;
	GLOBAL_POSITION_CMD = positionCmd;
	GLOBAL_BUS_VOLTAGE = busVoltage;
	GLOBAL_ROTOR_ANGLE_RAD = angleInRadians;
	GLOBAL_ROTOR_ANGLE = rotorAngle;
	GLOBAL_ROTOR_SPEED = rotorSpeed;
	GLOBAL_multiturn = multiturn;
	GLOBAL_PHASE_CURRENT_A = phaseCurrents.a;
	GLOBAL_PHASE_CURRENT_B = phaseCurrents.b;
	GLOBAL_PHASE_CURRENT_C = phaseCurrents.c;
}

void MotorControl::CheckConfigUpdates()
{
	if(systemData.realtimeData.isConfigChanged == true)
	{
		systemData.realtimeData.isConfigChanged = false;

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

float MotorControl::RotorAngleInCountsToElectricalAngleInRadians(int rotorAngleInCounts, uint8_t motorPoles)
{
	int angle = (rotorAngleInCounts >> 2) + systemData.configurationData.motor.motorEncoderOffset;
	angle = angle % (4096 / motorPoles);

	float angleInRadians = ((float)angle / (4096 / motorPoles)) * 2.0 * M_PI;
	return angleInRadians;
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

static inline float clampf(float x, float lo, float hi)
{
    return (x < lo) ? lo : (x > hi) ? hi : x;
}

void MotorControl::UpdateSVPWM(float v_alpha, float v_beta)
{
    const float sq3 = 1.7320508f;

    /* ── Inverse Clarke: αβ → three-phase references ─────────────────── */
    float va =  v_alpha;
    float vb = (-v_alpha + sq3 * v_beta) * 0.5f;
    float vc = (-v_alpha - sq3 * v_beta) * 0.5f;

    /* ── Find min and max ─────────────────────────────────────────────── */
    float v_max = va; if (vb > v_max) v_max = vb; if (vc > v_max) v_max = vc;
    float v_min = va; if (vb < v_min) v_min = vb; if (vc < v_min) v_min = vc;

    /* ── Overmodulation guard: rescale into the hexagon ──────────────── *
     *  The span (v_max - v_min) equals T1+T2. If > 1 we are outside the  *
     *  hexagon; scale all three phases down proportionally.               */
    float span = v_max - v_min;
    if (span > 1.0f)
    {
        float inv_span = 1.0f / span;
        va *= inv_span;
        vb *= inv_span;
        vc *= inv_span;
        v_max *= inv_span;
        v_min *= inv_span;
    }

    /* ── Zero-sequence offset (k-parameterised) ──────────────────────── */
    float k      = s_k;
    float offset = k - v_min - k * (v_max - v_min);

    /* ── Final duties [0, 1] ─────────────────────────────────────────── */
    float da = clampf(va + offset, 0.0f, 1.0f);
    float db = clampf(vb + offset, 0.0f, 1.0f);
    float dc = clampf(vc + offset, 0.0f, 1.0f);

    if(armed == true)
    {
    	motorPwm.SetPwmChannel1Duty((uint16_t)(da * (float)Common::MOTOR_PWM_MAX_CNT));
    	motorPwm.SetPwmChannel3Duty((uint16_t)(db * (float)Common::MOTOR_PWM_MAX_CNT));
    	motorPwm.SetPwmChannel2Duty((uint16_t)(dc * (float)Common::MOTOR_PWM_MAX_CNT));
    }
    else
    {
    	motorPwm.SetPwmChannel1Duty(0);
    	motorPwm.SetPwmChannel3Duty(0);
    	motorPwm.SetPwmChannel2Duty(0);
    }

	GLOBAL_PHASE_CURRENT_A_PWM = (uint16_t)(da * (float)Common::MOTOR_PWM_MAX_CNT);
	GLOBAL_PHASE_CURRENT_B_PWM = (uint16_t)(db * (float)Common::MOTOR_PWM_MAX_CNT);
	GLOBAL_PHASE_CURRENT_C_PWM = (uint16_t)(dc * (float)Common::MOTOR_PWM_MAX_CNT);
}
