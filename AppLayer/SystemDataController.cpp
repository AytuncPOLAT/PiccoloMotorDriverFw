#include "SystemDataController.hpp"
#include <string.h>

using namespace AppLayer;

void SystemDataController::OnCallback(uint8_t arg)
{
	if(communication.rxData.cmd == CMD_TYPE::PING)
	{
		userInterface.PingActivity();
	}

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
		storageController.ProgramNWords(0, &systemData.configurationData.flashMagicNumber,
				sizeof(systemData.configurationData), 32);
	}
}

SystemDataController::SystemDataController(Common::SystemData &systemDataRef,
										   Communication &communicationRef,
		                                   HardwareLayer::FlashStorage &storageControllerRef,
										   UserInterface& userInterfaceRef)
: systemData(systemDataRef)
, communication(communicationRef)
, storageController(storageControllerRef)
, userInterface(userInterfaceRef)
{
	communication.RegisterCallback(this);
	if(CheckIfConfigBlank() == true)
	{
		systemData.DefaultInitialization();
		storageController.ProgramNWords(0, &systemData.configurationData.flashMagicNumber, sizeof(systemData.configurationData), 32);
	}
	LoadSystemDataFromStorage();
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
			communication.txData.data0 = systemData.configurationData.flashMagicNumber;
			break;

		case Common::PROPERTY::SERIAL_NO:
			communication.txData.data0 = systemData.configurationData.deviceSerialNo;
			break;

		case Common::PROPERTY::FW_VERSION:
			communication.txData.data0 = systemData.configurationData.fwVersion;
			break;

		case Common::PROPERTY::DEV_ADDRESS:
			communication.txData.data0 = systemData.configurationData.deviceAddress;
			break;

		case Common::PROPERTY::DEV_MODE:
			communication.txData.data0 = systemData.configurationData.deviceMode;
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
			systemData.configurationData.deviceSerialNo = newValue;
			break;

		case Common::PROPERTY::FW_VERSION:
			systemData.configurationData.fwVersion = newValue;
			break;

		case Common::PROPERTY::DEV_ADDRESS:
			systemData.configurationData.deviceAddress = newValue;
			break;

		case Common::PROPERTY::DEV_MODE:
			systemData.configurationData.deviceMode = newValue;
			break;

		case Common::PROPERTY::PID_KP:
			systemData.configurationData.pidKp = (float)newValue;
			break;

		case Common::PROPERTY::PID_KI:
			systemData.configurationData.pidKi = (float)newValue;
			break;

		case Common::PROPERTY::PID_KD:
			systemData.configurationData.pidKd = (float)newValue;
			break;

		case Common::PROPERTY::PID_MAX_INTEGRAL_WU:
			systemData.configurationData.pidMaxIWindUp = (float)newValue;
			break;

		case Common::PROPERTY::PID_SAT:
			systemData.configurationData.pidSaturation = (float)newValue;
			break;
	}
}

