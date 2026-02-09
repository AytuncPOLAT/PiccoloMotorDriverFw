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
		systemData.runTimeData.isConfigChanged = true;
	}

	if(communication.rxData.cmd == CMD_TYPE::WRITE_TO_DEVICE_FLASH)
	{
		if(state == State::IDLE)
			state = State::FLASH_WRITE;
	}

	if(communication.rxData.cmd == CMD_TYPE::MOTION_COMMAND)
	{
		memcpy(&systemData.runTimeData.elecAngle, &communication.rxData.data0, sizeof(uint32_t));
		memcpy(&systemData.runTimeData.torque, &communication.rxData.data1, sizeof(uint32_t));
		memcpy(&systemData.runTimeData.speed, &communication.rxData.data2, sizeof(uint32_t));
		memcpy(&systemData.runTimeData.position, &communication.rxData.data3, sizeof(uint32_t));
	}

	if(communication.rxData.cmd == CMD_TYPE::READ_REALTIME)
	{
		communication.txData.cmd = CMD_TYPE::READ_REALTIME;
		communication.txData.address = systemData.configurationData.deviceAddress;
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

	case Common::PROPERTY::DEV_CONTROL_MODE:
		memcpy(&communication.txData.data0, &systemData.configurationData.controlMode, sizeof(uint32_t));
		break;

	//DQ Controller
	case Common::PROPERTY::PID_DQ_KP:
		memcpy(&communication.txData.data0, &systemData.configurationData.dqController.kp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_KI:
		memcpy(&communication.txData.data0, &systemData.configurationData.dqController.ki, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_KD:
		memcpy(&communication.txData.data0, &systemData.configurationData.dqController.kd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_MAX_INTEGRAL_WU:
		memcpy(&communication.txData.data0, &systemData.configurationData.dqController.maxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_SAT:
		memcpy(&communication.txData.data0, &systemData.configurationData.dqController.saturation, sizeof(uint32_t));
		break;

	//Speed Controller
	case Common::PROPERTY::PID_SPD_KP:
		memcpy(&communication.txData.data0, &systemData.configurationData.speedController.kp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_KI:
		memcpy(&communication.txData.data0, &systemData.configurationData.speedController.ki, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_KD:
		memcpy(&communication.txData.data0, &systemData.configurationData.speedController.kd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_MAX_INTEGRAL_WU:
		memcpy(&communication.txData.data0, &systemData.configurationData.speedController.maxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_SAT:
		memcpy(&communication.txData.data0, &systemData.configurationData.speedController.saturation, sizeof(uint32_t));
		break;

	//Position Controller
	case Common::PROPERTY::PID_POS_KP:
		memcpy(&communication.txData.data0, &systemData.configurationData.positionController.kp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_KI:
		memcpy(&communication.txData.data0, &systemData.configurationData.positionController.ki, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_KD:
		memcpy(&communication.txData.data0, &systemData.configurationData.positionController.kd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_MAX_INTEGRAL_WU:
		memcpy(&communication.txData.data0, &systemData.configurationData.positionController.maxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_SAT:
		memcpy(&communication.txData.data0, &systemData.configurationData.positionController.saturation, sizeof(uint32_t));
		break;

	// Motor Parameters
	case Common::PROPERTY::MOTOR_ENCODER_OFFSET:
			memcpy(&communication.txData.data0, &systemData.configurationData.motor.motorEncoderOffset, sizeof(uint32_t));
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

	case Common::PROPERTY::DEV_CONTROL_MODE:
		memcpy(&systemData.configurationData.controlMode, &newValue, sizeof(uint32_t));
		break;

	//DQ Controller
	case Common::PROPERTY::PID_DQ_KP:
		memcpy(&systemData.configurationData.dqController.kp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_KI:
		memcpy(&systemData.configurationData.dqController.ki, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_KD:
		memcpy(&systemData.configurationData.dqController.kd, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_MAX_INTEGRAL_WU:
		memcpy(&systemData.configurationData.dqController.maxIWindUp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_SAT:
		memcpy(&systemData.configurationData.dqController.saturation, &newValue, sizeof(uint32_t));
		break;

	//Speed Controller
	case Common::PROPERTY::PID_SPD_KP:
		memcpy(&systemData.configurationData.speedController.kp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_KI:
		memcpy(&systemData.configurationData.speedController.ki, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_KD:
		memcpy(&systemData.configurationData.speedController.kd, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_MAX_INTEGRAL_WU:
		memcpy(&systemData.configurationData.speedController.maxIWindUp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_SAT:
		memcpy(&systemData.configurationData.speedController.saturation, &newValue, sizeof(uint32_t));
		break;

	//Position Controller
	case Common::PROPERTY::PID_POS_KP:
		memcpy(&systemData.configurationData.positionController.kp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_KI:
		memcpy(&systemData.configurationData.positionController.ki, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_KD:
		memcpy(&systemData.configurationData.positionController.kd, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_MAX_INTEGRAL_WU:
		memcpy(&systemData.configurationData.positionController.maxIWindUp, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_SAT:
		memcpy(&systemData.configurationData.positionController.saturation, &newValue, sizeof(uint32_t));
		break;


	case Common::PROPERTY::MOTOR_ENCODER_OFFSET:
		memcpy(&systemData.configurationData.motor.motorEncoderOffset, &newValue, sizeof(uint32_t));
		break;
	}
}

