#include "MotorControl.hpp"
#include "SystemError.hpp"
#include "MotorPwm.hpp"
#include "SystemData.hpp"
#include "PidControl.hpp"
#include "AdcDriver.hpp"
#include "SignalProcessing.hpp"
#include "math.h"

namespace
{
	constexpr uint32_t COGGING_CALIBRATION_PASS_DURATION_MS = 15000U;
	constexpr uint32_t COGGING_CALIBRATION_MAX_PASS_DURATION_MS = 30000U;
}

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
}
void MotorControl::Init()
{
	// App construction loads flash configuration after MotorControl construction.
	armed = false;
	SetControllerParameters();
	CalculateMotorParameters();

	BaseType_t result = xTaskCreate(this->MotorControlTask, "MotorControlTask", 128 * 4, (void*) this,
			24, &taskHandle);
	if (result == pdPASS)
	{
		gMotorControlTaskHandle = taskHandle;
	}
}

void MotorControl::CalculateMotorParameters()
{
	electricalAngle.full = 4096 / systemData.configurationData.motor.motorPoles;
	electricalAngle.half = electricalAngle.full / 2;
	electricalAngle.quarter = electricalAngle.full / 4;
}

Common::ErrorType MotorControl::SetArmed(bool isArmed)
{
	if (isArmed && systemError != nullptr && systemError->HasError())
	{
		armed = false;
		return Common::ErrorType::UNDER_VOLTAGE;
	}
	armed = isArmed;
	if (isArmed && systemData.configurationData.controlMode ==
		static_cast<uint32_t>(Common::CONTROLLER_TYPE::POSITION))
	{
		positionCmd = multiturn;
	}
	return Common::ErrorType::OK;
}

Common::ErrorType MotorControl::DisarmAndClearErrors()
{
	armed = false;
	if (systemError != nullptr)
	{
		systemError->ClearError();
	}
	return Common::ErrorType::OK;
}

void MotorControl::SetSystemError(SystemError* errorPtr)
{
	systemError = errorPtr;
}

void MotorControl::SetDcMotor(int16_t duty)
{
	motorPwm.SetPwmChannel2Duty((Common::MOTOR_PWM_MAX_CNT/2) + duty/10.0);
	motorPwm.SetPwmChannel3Duty((Common::MOTOR_PWM_MAX_CNT/2) - duty/10.0);
}

void MotorControl::SetMotorCurrent(int32_t current)
{}

int32_t MotorControl::GetCoggingTorqueMean() const
{
	return coggingTorqueMean;
}

void MotorControl::SetCoggingTorqueMean(int32_t mean)
{
	coggingTorqueMean = mean;
}

float MotorControl::ApplyCoggingTorqueCancellation(float torqueCommand, uint16_t coggingMapIndex) const
{
	if (systemData.realtimeData.coggingCalibrationStatus != Common::COGGING_CALIBRATION_STATUS::COMPLETE)
	{
		return torqueCommand;
	}

	const float correctionScale = static_cast<float>(
		systemData.configurationData.antiCoggingTorquePercentage) / 100.0f;
	const float coggingTorque = static_cast<float>(
		systemData.realtimeData.coggingMap[coggingMapIndex] - coggingTorqueMean);
	return torqueCommand + coggingTorque * correctionScale;
}

void MotorControl::StartCoggingCalibration()
{
	coggingTorqueSum = 0;
	coggingTorqueMean = 0;
	coggingCalibrationStartTick = HAL_GetTick();
	coggingCalibrationStartPosition = multiturn;
	coggingForwardSampleCount = 0;
	coggingReverseSampleCount = 0;
	coggingReversePass = false;
	coggingReversePassStartElapsedMs = 0;
	coggingReversePassStartPosition = 0;
	for (uint16_t index = 0; index < Common::COGGING_MAP_SIZE; ++index)
	{
		systemData.realtimeData.coggingMap[index] = INT32_MIN;
	}
	for (uint16_t index = 0; index < sizeof(coggingReverseSampled); ++index)
	{
		coggingReverseSampled[index] = 0;
	}
	positionCommandFilter.Reset(static_cast<float>(multiturn));
	systemData.realtimeData.position = multiturn;
	systemData.realtimeData.coggingCalibrationProgress = 0;
	systemData.realtimeData.coggingCalibrationError = Common::COGGING_CALIBRATION_ERROR::NONE;
	systemData.realtimeData.coggingCalibrationStatus = Common::COGGING_CALIBRATION_STATUS::RUNNING;
	coggingCalibrationActive = true;
}

uint32_t MotorControl::UpdateCoggingCalibration(bool calibrationMode)
{
	if (coggingCalibrationActive && !calibrationMode)
	{
		systemData.realtimeData.coggingCalibrationStatus = Common::COGGING_CALIBRATION_STATUS::FAILED;
		systemData.realtimeData.coggingCalibrationError = Common::COGGING_CALIBRATION_ERROR::INTERRUPTED;
		coggingCalibrationActive = false;
	}

	if (calibrationMode && !coggingCalibrationActive)
	{
		StartCoggingCalibration();
	}

	if (!calibrationMode || !coggingCalibrationActive)
	{
		return 0;
	}

	const uint32_t elapsedMs = HAL_GetTick() - coggingCalibrationStartTick;
	if (!coggingReversePass && elapsedMs >= COGGING_CALIBRATION_PASS_DURATION_MS &&
		coggingForwardSampleCount == Common::COGGING_MAP_SIZE)
	{
		coggingReversePass = true;
		coggingReversePassStartElapsedMs = elapsedMs;
		coggingReversePassStartPosition = multiturn;
	}
	return elapsedMs;
}

void MotorControl::SetCoggingCalibrationMotion(uint32_t elapsedMs)
{
	const uint32_t passStartElapsedMs = coggingReversePass ? coggingReversePassStartElapsedMs : 0U;
	const uint32_t elapsedInPassMs = elapsedMs - passStartElapsedMs;
	const uint32_t passElapsedMs = elapsedInPassMs > COGGING_CALIBRATION_MAX_PASS_DURATION_MS ?
		COGGING_CALIBRATION_MAX_PASS_DURATION_MS : elapsedInPassMs;
	const int32_t travelCounts = static_cast<int32_t>(
		(static_cast<int64_t>(Common::ENCODER_COUNTS_PER_REV) * passElapsedMs) /
		COGGING_CALIBRATION_PASS_DURATION_MS);
	const int32_t passStartPosition = coggingReversePass ?
		coggingReversePassStartPosition : coggingCalibrationStartPosition;
	const int32_t targetPosition = passStartPosition + (coggingReversePass ? -travelCounts : travelCounts);
	systemData.realtimeData.position = targetPosition;
	positionCmd = static_cast<float>(targetPosition);
	speedCommand = positionController.Calculate(multiturn, positionCmd);
}

void MotorControl::RecordCoggingCalibrationSample(uint16_t coggingMapIndex)
{
	auto& coggingMap = systemData.realtimeData.coggingMap;
	if (!coggingReversePass && coggingMap[coggingMapIndex] == INT32_MIN)
	{
		coggingMap[coggingMapIndex] = static_cast<int32_t>(torqueCommand);
		++coggingForwardSampleCount;
		++systemData.realtimeData.coggingCalibrationProgress;
	}
	else if (coggingReversePass)
	{
		const uint8_t mask = static_cast<uint8_t>(1U << (coggingMapIndex & 7U));
		uint8_t& sampled = coggingReverseSampled[coggingMapIndex >> 3U];
		if ((sampled & mask) == 0U && coggingMap[coggingMapIndex] != INT32_MIN)
		{
			const int64_t torqueAverage = (static_cast<int64_t>(coggingMap[coggingMapIndex]) +
				static_cast<int32_t>(torqueCommand)) / 2;
			coggingMap[coggingMapIndex] = static_cast<int32_t>(torqueAverage);
			coggingTorqueSum += torqueAverage;
			sampled |= mask;
			++coggingReverseSampleCount;
			++systemData.realtimeData.coggingCalibrationProgress;
		}
	}
}

void MotorControl::FinishCoggingCalibration()
{
	coggingCalibrationActive = false;
	systemData.configurationData.controlMode = static_cast<uint32_t>(Common::CONTROLLER_TYPE::POSITION);
	systemData.realtimeData.position = coggingCalibrationStartPosition;
	positionCommandFilter.Reset(static_cast<float>(coggingCalibrationStartPosition));
	if (coggingReverseSampleCount == Common::COGGING_MAP_SIZE)
	{
		coggingTorqueMean = static_cast<int32_t>(coggingTorqueSum / Common::COGGING_MAP_SIZE);
		systemData.realtimeData.coggingCalibrationStatus = Common::COGGING_CALIBRATION_STATUS::COMPLETE;
	}
	else
	{
		systemData.realtimeData.coggingCalibrationStatus = Common::COGGING_CALIBRATION_STATUS::FAILED;
		systemData.realtimeData.coggingCalibrationError = Common::COGGING_CALIBRATION_ERROR::TRAVEL_LIMIT;
	}
}

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
			uint32_t controlMode = objectHandle->systemData.configurationData.controlMode;
			const bool coggingCalibrationMode = controlMode ==
				static_cast<uint32_t>(Common::CONTROLLER_TYPE::COGGING_CALIBRATION);
			const int rotorPositionCounts = objectHandle->rotorEncoder.GetPosition_Async();
			const uint16_t coggingMapIndex = static_cast<uint16_t>(
				(rotorPositionCounts >> 2) & (Common::COGGING_MAP_SIZE - 1U));

			objectHandle->phaseCurrents.a = objectHandle->analog.GetPhaseCurrent(0);
			objectHandle->phaseCurrents.b = objectHandle->analog.GetPhaseCurrent(1);
			objectHandle->phaseCurrents.c = objectHandle->analog.GetPhaseCurrent(2);
			objectHandle->busVoltage = objectHandle->analog.GetBusVoltage();

			if (objectHandle->systemError != nullptr && objectHandle->systemError->HasError())
			{
				objectHandle->armed = false;
			}

			objectHandle->angleInRadians = objectHandle->RotorAngleInCountsToElectricalAngleInRadians(rotorPositionCounts,
								objectHandle->systemData.configurationData.motor.motorPoles);

			objectHandle->multiturn = objectHandle->rotorEncoder.GetMultiTurnPosition();
			objectHandle->rotorSpeed = objectHandle->rotorEncoder.GetSpeed();

			objectHandle->positionCmd = 0.0f;
			const uint32_t calibrationElapsedMs = objectHandle->UpdateCoggingCalibration(coggingCalibrationMode);

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

				if (coggingCalibrationMode && objectHandle->coggingCalibrationActive)
				{
					objectHandle->SetCoggingCalibrationMotion(calibrationElapsedMs);
				}
				else if(!coggingCalibrationMode && controlMode
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
				else if (!coggingCalibrationMode)
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

				if (coggingCalibrationMode && objectHandle->coggingCalibrationActive)
				{
					objectHandle->RecordCoggingCalibrationSample(coggingMapIndex);
				}
				else if (controlMode >= static_cast<uint32_t>(Common::CONTROLLER_TYPE::TORQUE))
				{
					objectHandle->torqueCommand = objectHandle->ApplyCoggingTorqueCancellation(
						objectHandle->torqueCommand, coggingMapIndex);
				}

				const uint32_t reversePassElapsedMs = calibrationElapsedMs -
					objectHandle->coggingReversePassStartElapsedMs;
				const bool forwardPassTimedOut = !objectHandle->coggingReversePass &&
					calibrationElapsedMs >= COGGING_CALIBRATION_MAX_PASS_DURATION_MS &&
					objectHandle->coggingForwardSampleCount != Common::COGGING_MAP_SIZE;
				const bool reversePassFinished = objectHandle->coggingReversePass &&
					(objectHandle->coggingReverseSampleCount == Common::COGGING_MAP_SIZE ||
					 reversePassElapsedMs >= COGGING_CALIBRATION_MAX_PASS_DURATION_MS);
				const bool finishCoggingCalibration = coggingCalibrationMode &&
					objectHandle->coggingCalibrationActive &&
					(forwardPassTimedOut || reversePassFinished);
				if (finishCoggingCalibration && objectHandle->coggingCalibrationActive)
				{
					objectHandle->FinishCoggingCalibration();
				}

				if(controlMode >= (uint8_t) Common::CONTROLLER_TYPE::TORQUE)
				{
					auto torqueCommand = objectHandle->torqueCommand;

					if(objectHandle->systemData.configurationData.inverseTorque)
					{
						torqueCommand = -torqueCommand;
					}
					objectHandle->parkValues = objectHandle->TorqueLoop(torqueCommand,
							objectHandle->angleInRadians,
							objectHandle->phaseCurrents);
				}
				else if (controlMode
						>= (uint8_t) Common::CONTROLLER_TYPE::ELEC_ANGLE)
				{
					objectHandle->elecAngleCommand = objectHandle->systemData.realtimeData.elecAngle;
					objectHandle->parkValues = objectHandle->TorqueLoop(objectHandle->torqueCommand,
											(float)(objectHandle->elecAngleCommand / 585.0) * 2.0 * (float)M_PI,
											objectHandle->phaseCurrents);
				}

				if(controlMode
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
	rotorEncoder.SetDirectionInverted(systemData.configurationData.inverseEncoder != 0U);
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
