#include "SystemData.hpp"

Common::SystemData::SystemData()
{}

void Common::SystemData::DefaultInitialization()
{
	configurationData.flashMagicNumber = 0xBEEFBEEF;
	configurationData.deviceSerialNo = 1;
	configurationData.fwVersion = 1;
	configurationData.deviceAddress = 1;
	configurationData.deviceMode = 1;
	configurationData.pidKp = 1;
	configurationData.pidKi = 0;
	configurationData.pidKd = 0;
	configurationData.pidMaxIWindUp = 1000;
	configurationData.pidSaturation = 1000;
}
