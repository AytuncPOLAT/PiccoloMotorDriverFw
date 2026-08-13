#include "SystemDataController.hpp"
#include <string.h>

using namespace AppLayer;

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

	// TODO move into drv init
	objectHandle->drv.SendCommand(0, 0);
	objectHandle->drv.SetCurr2();
	objectHandle->drv.SetOCP();

	while(1)
	{
		if(objectHandle->state == State::FLASH_WRITE)
		{
			objectHandle->storageController.EraseUserSector();

			objectHandle->storageController.ProgramNWords(0, &objectHandle->systemData.configurationData.flashMagicNumber,
							sizeof(objectHandle->systemData.configurationData), 32);

			objectHandle->state = State::IDLE;
		}

		if(objectHandle->newRealtimePacket == true)
		{
			objectHandle->newRealtimePacket = false;

			int* const rtTargets[] = {
				&objectHandle->systemData.realtimeData.position,
				&objectHandle->systemData.realtimeData.speed,
				&objectHandle->systemData.realtimeData.torque,
			};
			const uint8_t rtIdx = static_cast<uint8_t>(objectHandle->communication.realtimeCommand.cmd)
								- static_cast<uint8_t>(CMD_TYPE::MOTION_POS_COMMAND);
			if (rtIdx < 3)
				memcpy(rtTargets[rtIdx], &objectHandle->communication.realtimeCommand.data[0], sizeof(uint32_t));
		}

		if(objectHandle->newPacket == true)
		{
			if(objectHandle->communication.rxData.cmd == CMD_TYPE::READ_FROM_DEVICE)
			{
				objectHandle->DataReadResponse((Common::PROPERTY)objectHandle->communication.rxData.data0);
				objectHandle->drv.ClearFaults();
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::WRITE_TO_DEVICE)
			{
				objectHandle->WriteToRam((Common::PROPERTY)objectHandle->communication.rxData.data0, objectHandle->communication.rxData.data1);
				objectHandle->systemData.realtimeData.isConfigChanged = true;
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::WRITE_TO_DEVICE_FLASH)
			{
				if(objectHandle->state == State::IDLE)
					objectHandle->state = State::FLASH_WRITE;
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::MOTION_COMMAND)
			{
				memcpy(&objectHandle->systemData.realtimeData.elecAngle, &objectHandle->communication.rxData.data0, sizeof(uint32_t));
				memcpy(&objectHandle->systemData.realtimeData.torque, &objectHandle->communication.rxData.data1, sizeof(uint32_t));
				memcpy(&objectHandle->systemData.realtimeData.speed, &objectHandle->communication.rxData.data2, sizeof(uint32_t));
				memcpy(&objectHandle->systemData.realtimeData.position, &objectHandle->communication.rxData.data3, sizeof(uint32_t));
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::READ_REALTIME)
			{
				objectHandle->communication.txData.cmd = CMD_TYPE::READ_REALTIME;
				objectHandle->communication.txData.address = objectHandle->systemData.configurationData.deviceAddress;
				objectHandle->communication.TransmitTxFrame();
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::DRIVER_ARM)
			{
				objectHandle->motorControl.SetArmed(true);
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::DRIVER_DISARM)
			{
				objectHandle->motorControl.SetArmed(false);
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::POSITION_HOME)
			{

			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::CURR_1)
			{
				objectHandle->drv.SetCurr1();
			}

			else if(objectHandle->communication.rxData.cmd == CMD_TYPE::CURR_2)
			{
				objectHandle->drv.SetCurr2();
			}

			objectHandle->newPacket = false;
		}
		osDelay(1);
	}
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

	auto& cfg = systemData.configurationData;
	auto& rt  = systemData.realtimeData;
	using P   = Common::PROPERTY;

	propertyMap[(uint8_t)P::FLASH_MAGIC]            = (uint32_t*)&cfg.flashMagicNumber;
	propertyMap[(uint8_t)P::SERIAL_NO]              = (uint32_t*)&cfg.deviceSerialNo;
	propertyMap[(uint8_t)P::FW_VERSION]             = (uint32_t*)&cfg.fwVersion;
	propertyMap[(uint8_t)P::DEV_ADDRESS]            = (uint32_t*)&cfg.deviceAddress;
	propertyMap[(uint8_t)P::DEV_CONTROL_MODE]       = (uint32_t*)&cfg.controlMode;
	propertyMap[(uint8_t)P::PID_DQ_KP]              = (uint32_t*)&cfg.dqController.kp;
	propertyMap[(uint8_t)P::PID_DQ_KI]              = (uint32_t*)&cfg.dqController.ki;
	propertyMap[(uint8_t)P::PID_DQ_KD]              = (uint32_t*)&cfg.dqController.kd;
	propertyMap[(uint8_t)P::PID_DQ_MAX_INTEGRAL_WU] = (uint32_t*)&cfg.dqController.maxIWindUp;
	propertyMap[(uint8_t)P::PID_DQ_SAT]             = (uint32_t*)&cfg.dqController.saturation;
	propertyMap[(uint8_t)P::PID_SPD_KP]             = (uint32_t*)&cfg.speedController.kp;
	propertyMap[(uint8_t)P::PID_SPD_KI]             = (uint32_t*)&cfg.speedController.ki;
	propertyMap[(uint8_t)P::PID_SPD_KD]             = (uint32_t*)&cfg.speedController.kd;
	propertyMap[(uint8_t)P::PID_SPD_MAX_INTEGRAL_WU]= (uint32_t*)&cfg.speedController.maxIWindUp;
	propertyMap[(uint8_t)P::PID_SPD_SAT]            = (uint32_t*)&cfg.speedController.saturation;
	propertyMap[(uint8_t)P::PID_POS_KP]             = (uint32_t*)&cfg.positionController.kp;
	propertyMap[(uint8_t)P::PID_POS_KI]             = (uint32_t*)&cfg.positionController.ki;
	propertyMap[(uint8_t)P::PID_POS_KD]             = (uint32_t*)&cfg.positionController.kd;
	propertyMap[(uint8_t)P::PID_POS_MAX_INTEGRAL_WU]= (uint32_t*)&cfg.positionController.maxIWindUp;
	propertyMap[(uint8_t)P::PID_POS_SAT]            = (uint32_t*)&cfg.positionController.saturation;
	propertyMap[(uint8_t)P::MOTOR_ENCODER_OFFSET]   = (uint32_t*)&cfg.motor.motorEncoderOffset;
	propertyMap[(uint8_t)P::MOTOR_POLES]            = (uint32_t*)&cfg.motor.motorPoles;
	propertyMap[(uint8_t)P::DC_BUS_VOLTAGE]         = (uint32_t*)&rt.dcBusVoltage;
	propertyMap[(uint8_t)P::MULTI_TURN_ENCODER]     = (uint32_t*)&rt.multiTurnEncoder;
	propertyMap[(uint8_t)P::MOTION_TELEMETRY]       = nullptr;
	propertyMap[(uint8_t)P::CURRENT_AMPLIFIER_GAIN] = (uint32_t*)&cfg.motor.currentAmplifierGain;
	propertyMap[(uint8_t)P::POSITION_HOME_MIN]      = (uint32_t*)&cfg.motor.positionHomeMin;
	propertyMap[(uint8_t)P::POSITION_HOME_MAX]      = (uint32_t*)&cfg.motor.positionHomeMax;
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
	communication.txData.cmd     = CMD_TYPE::READ_FROM_DEVICE;
	communication.txData.address = systemData.configurationData.deviceAddress;

	if (property == Common::PROPERTY::MOTION_TELEMETRY)
	{
		memcpy(&communication.txData.data0, &systemData.realtimeData.multiTurnEncoder, sizeof(int32_t));
		memcpy(&communication.txData.data1, &systemData.realtimeData.speedGet,         sizeof(int32_t));
		memcpy(&communication.txData.data2, &systemData.realtimeData.torqueGet,        sizeof(int32_t));
		memcpy(&communication.txData.data3, &systemData.realtimeData.motorCurrent,     sizeof(int32_t));
	}
	else
	{
		memcpy(&communication.txData.data0, propertyMap[static_cast<uint8_t>(property)], sizeof(uint32_t));
	}

	communication.TransmitTxFrame();
}

void SystemDataController::WriteToRam(Common::PROPERTY property, uint32_t newValue)
{
	// FLASH_MAGIC and DC_BUS_VOLTAGE are read-only over the bus; MOTION_TELEMETRY has no single write target
	if (property == Common::PROPERTY::FLASH_MAGIC    ||
	    property == Common::PROPERTY::DC_BUS_VOLTAGE ||
	    property == Common::PROPERTY::MOTION_TELEMETRY)
		return;

	memcpy(propertyMap[static_cast<uint8_t>(property)], &newValue, sizeof(uint32_t));
}

