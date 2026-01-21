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

	class PidController;

	class MotorControl
	{
	public:
		MotorControl(HardwareLayer::MotorPwm& motorPwmRef,
					 PidController& pidControllerRef,
					 HardwareLayer::AdcDriver& adcRef,
					 Common::SystemData& systemDataRef);

		void SetDcMotor(int16_t duty);
		Common::ErrorType SetArmed(bool isArmed);
		void Init();
		void SetMotorCurrent(int32_t current);
		void SetElectricalAngle(int16_t amplitude, float angle);

	private:
		BaseType_t taskHandle;
		static void MotorControlTask(void *argument);

		AlphaBetaZero ClarkTransform(ABC input);
		AlphaBetaZero clarkTransformResult;

		ABC InverseClarkeTransform(AlphaBetaZero input);

		DQZero ParkTransform(float alpha, float beta, float angleInRad);
		DQZero parkTransformResult;

		AlphaBetaZero InverseParkTransform(DQZero input, float theta);


		Common::MotorMode mode;
		bool armed = false;
		HardwareLayer::MotorPwm& motorPwm;

		int16_t motorCurrent = 0;

		PidController& pidController;
		HardwareLayer::AdcDriver& adc;

		SinusPwm sinPwm;
		Common::SystemData& systemData;


	};
}

#endif
