#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include "UsbVirtualCom.hpp"
#include "MotorPwm.hpp"
#include "AdcDriver.hpp"
#include "FlashMemoryController.hpp"
#include "DRV8316R_SpiDriver.hpp"
#include "QuadraticEncoderDriver.hpp"

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
	HardwareLayer::AdcDriver adc;
	HardwareLayer::FlashStorage flashStorage;
	TIM_HandleTypeDef timer1Handle;
	Drv8316rSpiDriver drv8316;
	HardwareLayer::QuadraticEncoderDriver externalQuadEncoder;

private:
	void SystemClockConfig();
	void PeriphCommonClockConfig();
};

#endif // HARDWARE_HPP
