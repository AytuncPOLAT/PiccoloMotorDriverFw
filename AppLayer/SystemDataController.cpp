#include "SystemDataController.hpp"

using namespace AppLayer;

void SystemDataController::OnCallback(uint8_t arg)
{
	if(communication.rxData.cmd == CMD_TYPE::WRITE_TO_DEVICE)
	{
		communication.txData.cmd = CMD_TYPE::READ_FROM_DEV;
		communication.txData.address = systemData.configurationData.deviceAddress;
		communication.txData.data0 = 2048;
		communication.Plot(2048);
	}
}

SystemDataController::SystemDataController(Common::SystemData &systemDataRef,
										   Communication &communicationRef,
		                                   HardwareLayer::FlashStorage &storageControllerRef)
: systemData(systemDataRef)
, communication(communicationRef)
, storageController(storageControllerRef)
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

