#include "SystemData.hpp"

Common::SystemData::SystemData()
{}

void Common::SystemData::DefaultInitialization()
{
	configurationData.flashMagicNumber = 0xBEEFBEEF;
	configurationData.deviceSerialNo = 1;
	configurationData.fwVersion = 2;
	configurationData.deviceAddress = 3;
	configurationData.deviceMode = 4;
	configurationData.pidKp = 5;
	configurationData.pidKi = 6;
	configurationData.pidKd = 7;
	configurationData.pidMaxIWindUp = 8;
	configurationData.pidSaturation = 9;
	configurationData.padding32[0] = 10;
	configurationData.padding32[1] = 11;
	configurationData.padding32[2] = 12;
	configurationData.padding32[3] = 13;
	configurationData.padding32[4] = 14;
	configurationData.padding16[0] = 15;
	configurationData.crc16 = 16;
}
