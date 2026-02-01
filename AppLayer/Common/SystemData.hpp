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

	constexpr float DC_BUS_SENSE_LOW_SIDE = 1000U;
	constexpr float DC_BUS_SENSE_HIGH_SIDE = 47000U;
	constexpr float DC_BUS_SENSE_RATIO = DC_BUS_SENSE_LOW_SIDE / (DC_BUS_SENSE_LOW_SIDE + DC_BUS_SENSE_HIGH_SIDE);

	enum class ADC_CHANNELS : uint8_t
	{
		DC_BUS_VOLTAGE = 4,
		PHASE_A_CURRENT,
		PHASE_B_CURRENT,
		PHASE_C_CURRENT,
		EXT0 = 5,
		EXT1 = 3
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
		PID_KP,
		PID_KI,
		PID_KD,
		PID_MAX_INTEGRAL_WU,
		PID_SAT,
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
		int pidKp;
		int pidKi;
		int pidKd;
		int pidMaxIWindUp;
		int pidSaturation;
		uint32_t padding32[5];
		uint16_t padding16[1];
		uint16_t crc16;
	};

	struct RunTimeData
	{
		int pwm;
		int torque;
		int speed;
		int position;
		bool isConfigChanged;
	};

	static_assert(sizeof(ConfigurationData) % 8 == 0, "Configuration data struct alignment error"
			", Configuration data must be aligned to 8 bytes");

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
