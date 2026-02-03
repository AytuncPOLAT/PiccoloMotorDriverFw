#include "SystemDataController.hpp"
#include <string.h>

using namespace AppLayer;

void SystemDataController::OnCallback(uint8_t arg)
{
	if(communication.rxData.cmd == CMD_TYPE::READ_FROM_DEVICE)
	{
		DataReadResponse((Common::PROPERTY)communication.rxData.data0);
	}

	if(communication.rxData.cmd == CMD_TYPE::WRITE_TO_DEVICE)
	{
		WriteToRam((Common::PROPERTY)communication.rxData.data0, communication.rxData.data1);
	}

	if(communication.rxData.cmd == CMD_TYPE::WRITE_TO_DEVICE_FLASH)
	{
		if(state == State::IDLE)
			state = State::FLASH_WRITE;
	}

	if(communication.rxData.cmd == CMD_TYPE::MOTION_COMMAND)
	{
		systemData.runTimeData.isConfigChanged = true;

		if(communication.rxData.data0 == 0)
		{
			memcpy(&systemData.runTimeData.pwm, &communication.rxData.data1, sizeof(uint32_t));
		}

		if(communication.rxData.data0 == 1)
		{
			memcpy(&systemData.runTimeData.torque, &communication.rxData.data1, sizeof(uint32_t));
		}

		if(communication.rxData.data0 == 2)
		{
			memcpy(&systemData.runTimeData.speed, &communication.rxData.data1, sizeof(uint32_t));
		}

		if(communication.rxData.data0 == 3)
		{
			memcpy(&systemData.runTimeData.position, &communication.rxData.data1, sizeof(uint32_t));
		}
	}

	if(communication.rxData.cmd == CMD_TYPE::READ_REALTIME)
	{
		communication.txData.cmd = CMD_TYPE::READ_REALTIME;
		communication.txData.address = systemData.configurationData.deviceAddress;
		//communication.txData.data0 = systemData.runTimeData.current;
		communication.TransmitTxFrame();
	}

	if(communication.rxData.cmd == CMD_TYPE::CURR_1)
	{
		drv.SetCurr1();
	}

	if(communication.rxData.cmd == CMD_TYPE::CURR_2)
	{
		drv.SetCurr2();
	}
}

void SystemDataController::TaskThread(void *argument)
{
	SystemDataController *objectHandle = static_cast<SystemDataController*>(argument);

	objectHandle->drv.SendCommand(0, 0);
	objectHandle->drv.SetCurr2();
	while(1)
	{


		if(objectHandle->state == State::FLASH_WRITE)
		{
			objectHandle->storageController.EraseUserSector();

			objectHandle->storageController.ProgramNWords(0, &objectHandle->systemData.configurationData.flashMagicNumber,
							sizeof(objectHandle->systemData.configurationData), 32);

			objectHandle->state = State::IDLE;
		}

		osDelay(20);
	}
}

SystemDataController::SystemDataController(Common::SystemData &systemDataRef,
										   Communication &communicationRef,
		                                   HardwareLayer::FlashStorage &storageControllerRef,
										   UserInterface& userInterfaceRef,
										   Drv8316rSpiDriver& drvRef)
: systemData(systemDataRef)
, communication(communicationRef)
, storageController(storageControllerRef)
, userInterface(userInterfaceRef)
, drv(drvRef)
{
	communication.RegisterCallback(this);
	if(CheckIfConfigBlank() == true)
	{
		systemData.DefaultInitialization();
		storageController.ProgramNWords(0,
										&systemData.configurationData.flashMagicNumber,
										sizeof(systemData.configurationData), 32);
	}

	LoadSystemDataFromStorage();
}

void SystemDataController::Init()
{
	taskHandle = xTaskCreate(this->TaskThread, "SystemDataControllerTask", 128 * 4, (void*) this,
			24, NULL);
}

bool SystemDataController::CheckIfConfigBlank()
{
	uint32_t magicNumber = 0;
	storageController.ReadFourBytes(0, &magicNumber);

	if(magicNumber == Common::FLASH_MAGIC_NUM)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool SystemDataController::LoadSystemDataFromStorage()
{
	return storageController.ReadNBytes(0, (uint32_t*)&systemData.configurationData.flashMagicNumber, sizeof(systemData.configurationData), 4)
			== Common::ErrorType::OK;
}

void SystemDataController::DataReadResponse(Common::PROPERTY property)
{
	communication.txData.cmd = CMD_TYPE::READ_FROM_DEVICE;
	communication.txData.address = systemData.configurationData.deviceAddress;

	switch(property)
	{
	case Common::PROPERTY::FLASH_MAGIC:
		memcpy(&communication.txData.data0, &systemData.configurationData.flashMagicNumber, sizeof(uint32_t));
		break;

	case Common::PROPERTY::SERIAL_NO:
		memcpy(&communication.txData.data0, &systemData.configurationData.deviceSerialNo, sizeof(uint32_t));
		break;

	case Common::PROPERTY::FW_VERSION:
		memcpy(&communication.txData.data0, &systemData.configurationData.fwVersion, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DEV_ADDRESS:
		memcpy(&communication.txData.data0, &systemData.configurationData.deviceAddress, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DEV_MODE:
		memcpy(&communication.txData.data0, &systemData.configurationData.deviceMode, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_KP:
		memcpy(&communication.txData.data0, &systemData.configurationData.pidKp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_KI:
		memcpy(&communication.txData.data0, &systemData.configurationData.pidKi, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_KD:
		memcpy(&communication.txData.data0, &systemData.configurationData.pidKd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_MAX_INTEGRAL_WU:
		memcpy(&communication.txData.data0, &systemData.configurationData.pidMaxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SAT:
		memcpy(&communication.txData.data0, &systemData.configurationData.pidSaturation, sizeof(uint32_t));
		break;
	}

	communication.TransmitTxFrame();
}

void SystemDataController::WriteToRam(Common::PROPERTY property, uint32_t newValue)
{
	switch(property)
	{
	case Common::PROPERTY::FLASH_MAGIC:
		//systemData.configurationData.flashMagicNumber;
		break;

	case Common::PROPERTY::SERIAL_NO:
		memcpy(&systemData.configurationData.deviceSerialNo, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::FW_VERSION:
		memcpy(&systemData.configurationData.fwVersion, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DEV_ADDRESS:
		memcpy(&systemData.configurationData.deviceAddress, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DEV_MODE:
		memcpy(&systemData.configurationData.deviceMode, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_KP:
		memcpy(&systemData.configurationData.pidKp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_KI:
		memcpy(&systemData.configurationData.pidKi, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_KD:
		memcpy(&systemData.configurationData.pidKd, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_MAX_INTEGRAL_WU:
		memcpy(&systemData.configurationData.pidMaxIWindUp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SAT:
		memcpy(&systemData.configurationData.pidSaturation, &newValue, sizeof(uint32_t));
		break;
	}
}

