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

	class MotorControl
	: public HardwareLayer::IEncoder::Callback
	{
	public:
		MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
					 AnalogProcessor& analogRef,
					 Common::SystemData& systemDataRef,
					 HardwareLayer::IEncoder& rotorEncoderRef);

		void SetDcMotor(int16_t duty);
		Common::ErrorType SetArmed(bool isArmed);
		void Init();
		void SetMotorCurrent(int32_t current);
		void OnIndexPulseCallBack();

	private:
		void CheckConfigUpdates();
		static void MotorControlTask(void *argument);

		AlphaBetaZero ClarkTransform(ABC input);
		ABC InverseClarkeTransform(AlphaBetaZero input);
		DQZero ParkTransform(AlphaBetaZero abz, float angleInRad);
		AlphaBetaZero InverseParkTransform(DQZero input, float theta);
		void TorqueLoop(float setTorque, float angleInRadians, ABC phaseCurrents);
		float SpeedLoop(float setSpeed, float speedFb);
		void SetControllerParameters();

		BaseType_t taskHandle;
		HardwareLayer::MotorPwm& motorPwm;
		AnalogProcessor& analog;
		HardwareLayer::IEncoder& rotorEncoder;
		Common::SystemData& systemData;
		Common::MotorMode mode;
		bool armed = false;

		ABC phaseCurrents;
		PidController dController, qController, speedController, positionController;

		int elecAngleCommand;
		float torqueCommand;
		float speedCommand;
		int positionCommand;
	};
}

#endif
