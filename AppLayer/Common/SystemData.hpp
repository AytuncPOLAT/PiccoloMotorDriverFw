#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h>
namespace Common
{
	const uint32_t FLASH_MAGIC_NUM = 0xBEEFBEEF;
	const uint32_t RS485_BAUD = 115200;
	const uint16_t PARAMETER_LEN = 32;
	const uint16_t MOTOR_POLES = 7;
	const uint16_t COUNT_PER_REV = 4095;
	const uint16_t MOTOR_PWM_MAX_CNT = 1000;
	
	enum class DATA_TYPE
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
		float pidKp;
		float pidKi;
		float pidKd;
		float pidMaxIWindUp;
		float pidSaturation;
		uint32_t padding32[5];
		uint16_t padding16[1];
		uint16_t crc16;
	};

	static_assert(sizeof(ConfigurationData) % 8 == 0, "Configuration data struct alignment error"
			", Configuration data must be aligned to 8 bytes");

	class SystemData
	{
	public:
		SystemData();
		void DefaultInitialization();

		ConfigurationData configurationData;
	private:
	};
}
#endif // SYSTEM_DATA_H
