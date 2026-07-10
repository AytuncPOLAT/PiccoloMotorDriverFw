#include "SystemData.hpp"



Common::SystemData::SystemData()
{}

void Common::SystemData::DefaultInitialization()
{
	configurationData.flashMagicNumber = 0xBEEFBEEF;
	configurationData.deviceSerialNo = 1U;
	configurationData.fwVersion = 2U;
	configurationData.deviceAddress = 3U;
	configurationData.controlMode = 4U;
	configurationData.adcPhase_A_Offset = 0;
	configurationData.adcPhase_A_Gain = 0;
	configurationData.adcPhase_B_Offset = 0;
	configurationData.adcPhase_B_Gain = 0;
	configurationData.adcPhase_C_Offset = 0;
	configurationData.adcPhase_C_Gain = 0;

	configurationData.dqController.kp = 10000;
	configurationData.dqController.ki = 1000;
	configurationData.dqController.kd = 0;
	configurationData.dqController.maxIWindUp = 10000;
	configurationData.dqController.saturation = 500000;

	configurationData.speedController.kp = 1000;
	configurationData.speedController.ki = 100;
	configurationData.speedController.kd = 0;
	configurationData.speedController.maxIWindUp = 100000;
	configurationData.speedController.saturation = 500000;

	configurationData.positionController.kp = 1000;
	configurationData.positionController.ki = 100;
	configurationData.positionController.kd = 0;
	configurationData.positionController.maxIWindUp = 100000;
	configurationData.positionController.saturation = 500000;

	configurationData.motor.motorEncoderOffset = 0;
	configurationData.motor.motorPoles = Common::MOTOR_POLES;
	configurationData.motor.currentAmplifierGain = 0;
	configurationData.motor.positionHomeMin = -1000000;
	configurationData.motor.positionHomeMax = 1000000;
	
	configurationData.crc16 = 16U;
}
