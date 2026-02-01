#include "SystemData.hpp"

Common::SystemData::SystemData()
{}

void Common::SystemData::DefaultInitialization()
{
	configurationData.flashMagicNumber = 0xBEEFBEEF;
	configurationData.deviceSerialNo = 1U;
	configurationData.fwVersion = 2U;
	configurationData.deviceAddress = 3U;
	configurationData.deviceMode = 4U;
	configurationData.adcPhase_A_Offset = 0;
	configurationData.adcPhase_A_Gain = 0;
	configurationData.adcPhase_B_Offset = 0;
	configurationData.adcPhase_B_Gain = 0;
	configurationData.adcPhase_C_Offset = 0;
	configurationData.adcPhase_C_Gain = 0;
	configurationData.pidKp = 1000U;
	configurationData.pidKi = 2000U;
	configurationData.pidKd = 3000U;
	configurationData.pidMaxIWindUp = 4000U;
	configurationData.pidSaturation = 5000U;
	configurationData.padding32[0] = 10U;
	configurationData.padding32[1] = 11U;
	configurationData.padding32[2] = 12U;
	configurationData.padding32[3] = 13U;
	configurationData.padding32[4] = 14U;
	configurationData.padding16[0] = 15U;
	configurationData.crc16 = 16U;
}
