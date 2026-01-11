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
	configurationData.pidKp = 5.5f;
	configurationData.pidKi = 6.6f;
	configurationData.pidKd = 7.7f;
	configurationData.pidMaxIWindUp = 8.8f;
	configurationData.pidSaturation = 9.9f;
	configurationData.padding32[0] = 10U;
	configurationData.padding32[1] = 11U;
	configurationData.padding32[2] = 12U;
	configurationData.padding32[3] = 13U;
	configurationData.padding32[4] = 14U;
	configurationData.padding16[0] = 15U;
	configurationData.crc16 = 16U;
}
