#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h>
#include <math.h>

namespace Common
{
	const uint32_t FLASH_MAGIC_NUM = 0xBEEFBEEF;
	const uint32_t RS485_BAUD = 115200;
	const uint16_t PARAMETER_LEN = 32;
	const uint16_t MOTOR_POLES = 7;
	const uint16_t COUNT_PER_REV = 4095;
	const uint16_t MOTOR_PWM_MAX_CNT = 1000;

	constexpr uint8_t ADC_RESOLUTION_IN_BITS = 12U;
	constexpr int16_t ADC_MAX = pow(2U, ADC_RESOLUTION_IN_BITS);
	constexpr int16_t ADC_MID_POINT = ADC_MAX / 2U;
	constexpr float ADC_REF_IN_MILLI_VOLTS = 3300U;
	constexpr float MILLIVOLTS_PER_COUNT = ADC_REF_IN_MILLI_VOLTS / ADC_MAX;
	constexpr float MILLIAMPS_PER_COUNT = MILLIVOLTS_PER_COUNT / 1.2;

	constexpr float DC_BUS_SENSE_LOW_SIDE = 1000U;
	constexpr float DC_BUS_SENSE_HIGH_SIDE = 47000U;
	constexpr float DC_BUS_SENSE_RATIO = DC_BUS_SENSE_LOW_SIDE / (DC_BUS_SENSE_LOW_SIDE + DC_BUS_SENSE_HIGH_SIDE);

	enum class ADC_CHANNELS : uint8_t
	{
		PHASE_A_CURRENT = 0,
		PHASE_B_CURRENT = 1,
		PHASE_C_CURRENT = 2,
		EXT1 = 3,
		DC_BUS_VOLTAGE = 4,
		EXT0 = 5,
	};

	enum class DATA_TYPE : uint8_t
	{
		U_INTEGER = 0,
		S_INTEGER,
		FLOATING
	};

	enum class PROPERTY : uint8_t
	{
		FLASH_MAGIC = 0,
		SERIAL_NO,
		FW_VERSION,
		DEV_ADDRESS,
		DEV_MODE,
		PID_DQ_KP,
		PID_DQ_KI,
		PID_DQ_KD,
		PID_DQ_MAX_INTEGRAL_WU,
		PID_DQ_SAT,

		PID_SPD_KP,
		PID_SPD_KI,
		PID_SPD_KD,
		PID_SPD_MAX_INTEGRAL_WU,
		PID_SPD_SAT,
	};

	struct __attribute__((packed)) ConfigurationData
	{
		uint32_t flashMagicNumber;
		uint32_t deviceSerialNo;
		uint32_t fwVersion;
		uint32_t deviceAddress;
		uint32_t deviceMode;
		int adcPhase_A_Offset;
		int adcPhase_A_Gain;
		int adcPhase_B_Offset;
		int adcPhase_B_Gain;
		int adcPhase_C_Offset;
		int adcPhase_C_Gain;

		struct __attribute__((packed)) SpeedController
		{
			int kp;
			int ki;
			int kd;
			int maxIWindUp;
			int saturation;
		}speedController;

		struct __attribute__((packed)) DqController //Torque
		{
			int kp;
			int ki;
			int kd;
			int maxIWindUp;
			int saturation;
		}dqController;

		//uint32_t padding32[0];
		uint16_t padding16[1];
		uint16_t crc16;
	};

	static_assert(sizeof(ConfigurationData) % 8 == 0, "Configuration data struct alignment error"
			", Configuration data must be aligned to 8 bytes");

	struct RunTimeData
	{
		int pwm;
		int torque;
		int speed;
		int position;
		bool isConfigChanged;
	};

	class SystemData
	{
	public:
		SystemData();
		void DefaultInitialization();

		ConfigurationData configurationData;
		RunTimeData runTimeData;
	private:
	};
}
#endif // SYSTEM_DATA_H
