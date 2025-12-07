#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include "UsbVirtualCom.hpp"
#include "MotorPwm.hpp"

extern "C"
{
#include "main.h"
#include "cmsis_os.h"
}

class Hardware
{
public:
	Hardware();

	HardwareLayer::UsbVirtualCom usbCom;
	HardwareLayer::MotorPwm motorPwm;
private:
	void SystemClockConfig();
	void PeriphCommonClockConfig();
};

#endif // HARDWARE_HPP
