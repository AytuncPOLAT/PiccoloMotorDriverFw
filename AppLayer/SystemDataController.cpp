#include "SystemDataController.hpp"
#include <string.h>

using namespace AppLayer;

namespace Common
{
	QueueHandle_t remoteCommandQueue = NULL;
}

void SystemDataController::OnCallback(uint8_t arg)
{
	if(arg == 222)
		newPacket = true;

	else if(arg == 111)
		newRealtimePacket = true;
}

void SystemDataController::TaskThread(void *argument)
{
	SystemDataController *objectHandle = static_cast<SystemDataController*>(argument);

	Common::RemoteCommand remoteCommand;

	// TODO move into drv init
	objectHandle->drv.SendCommand(0, 0);
	objectHandle->drv.SetCurr2();
	objectHandle->drv.SetOCP();

	while(1)
	{
		if(xQueueReceive(Common::remoteCommandQueue, &remoteCommand, portMAX_DELAY) == pdTRUE)
		{
			switch((Common::CMD_TYPE)remoteCommand.command)
			{
			case Common::CMD_TYPE::READ_FROM_DEVICE:
				objectHandle->DataReadResponse((Common::PROPERTY)remoteCommand.registerAddress);
				break;

			case Common::CMD_TYPE::WRITE_TO_DEVICE:
				objectHandle->WriteToRam((Common::PROPERTY)remoteCommand.registerAddress,
				*(uint32_t*)remoteCommand.data);
				break;

			case Common::CMD_TYPE::WRITE_TO_DEVICE_FLASH:
				if(objectHandle->state == State::IDLE)
					objectHandle->state = State::FLASH_WRITE;
				break;

			case Common::CMD_TYPE::MOTION_POS_COMMAND:
				memcpy(&objectHandle->systemData.realtimeData.position, &remoteCommand.data[0], sizeof(uint32_t));
				break;

			case Common::CMD_TYPE::MOTION_SPEED_COMMAND:
				memcpy(&objectHandle->systemData.realtimeData.speed, &remoteCommand.data[0], sizeof(uint32_t));
				break;

			case Common::CMD_TYPE::MOTION_TORQUE_COMMAND:
				memcpy(&objectHandle->systemData.realtimeData.torque, &remoteCommand.data[0], sizeof(uint32_t));
				break;

			case Common::CMD_TYPE::DRIVER_ARM:
				objectHandle->motorControl.SetArmed(true);
				break;

			case Common::CMD_TYPE::DRIVER_DISARM:
				objectHandle->motorControl.SetArmed(false);
				break;
			default:
				break;
			}

			if(objectHandle->state == State::FLASH_WRITE)
			{
				objectHandle->storageController.EraseUserSector();

				objectHandle->storageController.ProgramNWords(0, &objectHandle->systemData.configurationData.flashMagicNumber,
								sizeof(objectHandle->systemData.configurationData), 32);

				objectHandle->state = State::IDLE;
			}
		}
	}


/*
			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::READ_REALTIME)
			{
				objectHandle->communication.txData.cmd = CMD_TYPE::READ_REALTIME;
				objectHandle->communication.txData.address = objectHandle->systemData.configurationData.deviceAddress;
				objectHandle->communication.TransmitTxFrame();
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::CURR_1)
			{
				objectHandle->drv.SetCurr1();
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::CURR_2)
			{
				objectHandle->drv.SetCurr2();
			}
*/
}

SystemDataController::SystemDataController(Common::SystemData &systemDataRef,
										   Communication &communicationRef,
		                                   HardwareLayer::FlashStorage &storageControllerRef,
										   UserInterface& userInterfaceRef,
										   Drv8316rSpiDriver& drvRef,
										   MotorControl& motorControlRef)
: systemData(systemDataRef)
, communication(communicationRef)
, storageController(storageControllerRef)
, userInterface(userInterfaceRef)
, drv(drvRef)
, motorControl(motorControlRef)
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
	communication.serialFrameTx.canFrame.command = Common::CMD_TYPE::READ_FROM_DEVICE;
	communication.serialFrameTx.canFrame.messageID = systemData.configurationData.deviceAddress;
	communication.serialFrameTx.canFrame.registerAddress = (uint8_t)property;

	switch(property)
	{
	case Common::PROPERTY::FLASH_MAGIC:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.flashMagicNumber, sizeof(uint32_t));
		break;

	case Common::PROPERTY::SERIAL_NO:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.deviceSerialNo, sizeof(uint32_t));
		break;

	case Common::PROPERTY::FW_VERSION:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.fwVersion, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DEV_ADDRESS:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.deviceAddress, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DEV_CONTROL_MODE:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.controlMode, sizeof(uint32_t));
		break;

	//DQ Controller
	case Common::PROPERTY::PID_DQ_KP:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.dqController.kp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_KI:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.dqController.ki, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_KD:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.dqController.kd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_MAX_INTEGRAL_WU:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.dqController.maxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_DQ_SAT:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.dqController.saturation, sizeof(uint32_t));
		break;

	//Speed Controller
	case Common::PROPERTY::PID_SPD_KP:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.speedController.kp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_KI:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.speedController.ki, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_KD:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.speedController.kd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_MAX_INTEGRAL_WU:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.speedController.maxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_SPD_SAT:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.speedController.saturation, sizeof(uint32_t));
		break;

	//Position Controller
	case Common::PROPERTY::PID_POS_KP:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.positionController.kp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_KI:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.positionController.ki, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_KD:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.positionController.kd, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_MAX_INTEGRAL_WU:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.positionController.maxIWindUp, sizeof(uint32_t));
		break;

	case Common::PROPERTY::PID_POS_SAT:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.positionController.saturation, sizeof(uint32_t));
		break;

	// Motor Parameters
	case Common::PROPERTY::MOTOR_ENCODER_OFFSET:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.motor.motorEncoderOffset, sizeof(uint32_t));
		break;

	case Common::PROPERTY::MOTOR_POLES:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.motor.motorPoles, sizeof(uint32_t));
		break;

	case Common::PROPERTY::DC_BUS_VOLTAGE:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.realtimeData.dcBusVoltage, sizeof(uint32_t));
		break;

	case Common::PROPERTY::MULTI_TURN_ENCODER:	
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.realtimeData.multiTurnEncoder, sizeof(int32_t));
		break;

	case Common::PROPERTY::MOTION_TELEMETRY:
		//memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.realtimeData.multiTurnEncoder, sizeof(int32_t));
		//memcpy(&communication.serialFrameTx.canFrame.data[1], &systemData.realtimeData.speedGet, sizeof(int32_t));
		//memcpy(&communication.serialFrameTx.canFrame.data[2], &systemData.realtimeData.torqueGet, sizeof(int32_t));
		//memcpy(&communication.serialFrameTx.canFrame.data[3], &systemData.realtimeData.motorCurrent, sizeof(int32_t));
		break;

	case Common::PROPERTY::CURRENT_AMPLIFIER_GAIN:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.motor.currentAmplifierGain, sizeof(uint32_t));
		break;

	case Common::PROPERTY::POSITION_HOME_MIN:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.motor.positionHomeMin, sizeof(uint32_t));
		break;

	case Common::PROPERTY::POSITION_HOME_MAX:
		memcpy(&communication.serialFrameTx.canFrame.data[0], &systemData.configurationData.motor.positionHomeMax, sizeof(uint32_t));
		break;
	}

	communication.Respond();
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

	case Common::PROPERTY::DC_BUS_VOLTAGE:
		// dcBusVoltage is updated by Telemetry and is read-only over the bus
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

	case Common::PROPERTY::MOTOR_POLES:
		memcpy(&systemData.configurationData.motor.motorPoles, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::MULTI_TURN_ENCODER:
		memcpy(&systemData.realtimeData.multiTurnEncoder, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::CURRENT_AMPLIFIER_GAIN:
		memcpy(&systemData.configurationData.motor.currentAmplifierGain, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::POSITION_HOME_MIN:
		memcpy(&systemData.configurationData.motor.positionHomeMin, &newValue, sizeof(uint32_t));
		break;

	case Common::PROPERTY::POSITION_HOME_MAX:
		memcpy(&systemData.configurationData.motor.positionHomeMax, &newValue, sizeof(uint32_t));
		break;
	}
}

