#ifndef SYSTEM_DATA_H
#define SYSTEM_DATA_H

#include <stdint.h>
namespace Common
{
	const uint32_t FLASH_MAGIC_NUM = 0xABCD1234;
	const uint32_t RS485_BAUD = 115200;
	const uint16_t PARAMETER_LEN = 32;
	const uint16_t MOTOR_POLES = 7;
	const uint16_t COUNT_PER_REV = 4095;

	enum CommandEnum
	{
		READ,
		WRITE,
		WRITE_TO_FLASH,
		TEST
	};

	enum ParameterEnum
	{
		SERIAL_NO = 1,
		FW_VER,
		DEVICE_ADDRESS,
		DEVICE_MODE,
		MOTOR_DIR,
		INVERTER_PERIOD,
		INVERTER_DEAD_TIME,
		MAX_MOTOR_DUTY,
		MOTOR_SPEED,
		PID_KP,
		PID_KI,
		PID_KD,
		PID_MAX_INTEGRAL_WINDUP,
		PID_MAX_PID_OUTPUT,
		QUAD_ENC_POS,
		QUAD_ENC_RESET,
        POS_CTRL_SET,
		COMMUTATION_OFFSET,
		PARAMETER_CRC
	};
	
	struct __attribute__((packed)) ConfigurationData
	{
		uint32_t parameter[PARAMETER_LEN];
		uint16_t crc;
	};


	const struct ConfigurationData defaultConfiguration = 
	{
    .parameter ={	0xABCD1234, //MagicNumber
					1234,		//SERIAL_NO
					100,		//FW_VER
					1,			//DEVICE_ADDRESS
					0,			//DEVICE_MODE
					0,			//MOTOR_DIR
					4096,		//INVERTER_PERIOD
					200,		//INVERTER_DEAD_TIME
					0,		//MAX_MOTOR_DUTY
					0,			//MOTOR_SPEED
					1000,		//PID_KP
					0,			//PID_KI
					0,			//PID_KD
					1000,		//PID_MAX_INTEGRAL_WINDUP
					0,		//PID_MAX_PID_OUTPUT
					0, //QUAD_ENC_POS
					0, //QUAD_ENC_RESET
					0, //COMMUTATION_OFFSET
					0, //POS_CTRL_SET
				},
	.crc = 0
	};
}
#endif // SYSTEM_DATA_H
