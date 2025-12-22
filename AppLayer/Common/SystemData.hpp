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
	
	struct __attribute__((packed)) ConfigurationData
	{
		uint32_t flashMagicNumber;
		uint32_t deviceSerialNo;
		uint32_t fwVersion;
		uint32_t deviceAddress;
		uint32_t deviceMode;
		uint32_t pidKp;
		uint32_t pidKi;
		uint32_t pidKd;
		uint32_t pidMaxIWindUp;
		uint32_t pidSaturation;
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
