#ifndef MOTOR_CONTROL_HPP
#define MOTOR_CONTROL_HPP

extern "C"
{
#include "main.h"
#include "cmsis_os.h"
}

#include "stdint.h"
#include "ErrorHandler.hpp"
#include "SinusPwm.hpp"
#include "SystemData.hpp"
#include "IEncoder.hpp"
#include "PidControl.hpp"
#include "AnalogProcessor.hpp"
#include "SignalProcessing.hpp"

namespace Common
{
	enum class MotorMode : uint8_t
	{
		DC_AB = 0,
		PMSM = 1,
	};
}

namespace HardwareLayer
{
	class MotorPwm;
	class AdcDriver;
}

namespace AppLayer
{
	struct AlphaBetaZero
	{
		float alpha;
		float beta;
		float zero;
	};

	struct DQZero
	{
		float d;
		float q;
		float zero;
	};

	struct ABC
	{
	    float a;
	    float b;
	    float c;
	};

	struct ElectricalAngle
	{
		float full;
		float half;
		float quarter;
	};

	class MotorControl
	: public HardwareLayer::IEncoder::Callback
	{
	public:
		MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
					 AnalogProcessor& analogRef,
					 Common::SystemData& systemDataRef,
					 HardwareLayer::IEncoder& rotorEncoderRef,
					 HardwareLayer::IEncoder& quadEncoderRef);

		void SetDcMotor(int16_t duty);
		Common::ErrorType SetArmed(bool isArmed);
		Common::ErrorType DisarmAndClearErrors();
		void Init();
		void SetMotorCurrent(int32_t current);
		int32_t GetCoggingTorqueMean() const;
		void SetCoggingTorqueMean(int32_t mean);
		void OnIndexPulseCallBack();
		void SetSystemError(class SystemError* errorPtr);

	private:
		void CheckConfigUpdates();
		float ApplyCoggingTorqueCancellation(float torqueCommand, uint16_t coggingMapIndex) const;
		void StartCoggingCalibration();
		uint32_t UpdateCoggingCalibration(bool calibrationMode);
		void SetCoggingCalibrationMotion(uint32_t elapsedMs);
		void RecordCoggingCalibrationSample(uint16_t coggingMapIndex);
		void FinishCoggingCalibration();
		static void MotorControlTask(void *argument);
		static void EncoderUpdater(void *argument);

		AlphaBetaZero ClarkTransform(ABC input);
		ABC InverseClarkeTransform(AlphaBetaZero input);
		DQZero ParkTransform(AlphaBetaZero abz, float angleInRad);
		AlphaBetaZero InverseParkTransform(DQZero input, float theta);
		DQZero TorqueLoop(float setTorque, float angleInRadians, ABC phaseCurrents);
		float SpeedLoop(float setSpeed, float speedFb);
		void DebugMonitor();
		void SetControllerParameters();
		
		float RotorAngleInCountsToElectricalAngleInRadians(int rotorAngleInCounts, uint8_t motorPoles);
		void CalculateMotorParameters();
		void UpdateSVPWM(float v_alpha, float v_beta);

		TaskHandle_t taskHandle;
		TaskHandle_t encoderTaskHandle;
		HardwareLayer::MotorPwm& motorPwm;
		AnalogProcessor& analog;
		HardwareLayer::IEncoder& rotorEncoder;
		HardwareLayer::IEncoder& quadEncoder;
		Common::SystemData& systemData;
		Common::MotorMode mode;
		bool armed = true;
		class SystemError* systemError = nullptr;
		
		ElectricalAngle electricalAngle;

		ABC phaseCurrents;
		PidController dController, qController, speedController, positionController;

		LowPassFilter positionCommandFilter;
		LowPassFilter parkDFilter;
		LowPassFilter parkQFilter;

		// Debug monitor variables
		float busVoltage;
		float rotorAngle;
		float angleInRadians;
		int multiturn;
		int telemetryMotorCurrent;
		int rotorSpeed;
		float positionCmd;
		DQZero parkValues;

		int elecAngleCommand;
		float torqueCommand;
		float speedCommand;
		int positionCommand;
		bool coggingCalibrationActive = false;
		bool coggingReversePass = false;
		uint32_t coggingCalibrationStartTick = 0;
		int32_t coggingCalibrationStartPosition = 0;
		uint32_t coggingReversePassStartElapsedMs = 0;
		int32_t coggingReversePassStartPosition = 0;
		uint16_t coggingForwardSampleCount = 0;
		uint16_t coggingReverseSampleCount = 0;
		uint8_t coggingReverseSampled[(Common::COGGING_MAP_SIZE + 7U) / 8U] = {};
		int64_t coggingTorqueSum = 0;
		int32_t coggingTorqueMean = 0;

	};
}

#endif
