#ifndef ANALOG_PROCESSOR_HPP
#define ANALOG_PROCESSOR_HPP

extern "C"
{
//#include "main.h"
#include "cmsis_os.h"
}

#include "AdcDriver.hpp"
#include "SystemData.hpp"
#include "DRV8316R_SpiDriver.hpp"
#include "SignalProcessing.hpp"

namespace AppLayer
{
	class AnalogProcessor
	{
	public:
		AnalogProcessor(HardwareLayer::AdcDriver& adcRef,
				Common::SystemData& systemDataRef,
				Drv8316rSpiDriver& drvRef);

		float GetPhaseCurrent(uint8_t channel);
		float GetBusVoltage();
		uint16_t GetExtAnalog(uint8_t channel);
		bool IsCalibrated();
		void SetVoltageCurrentRatio(float gain);
		void Init();
		void ResetConversionDoneFlag();
		bool GetConversionDoneFlag();
		void SetCurrentSenseGain();

	private:
		static void AnalogProcessTask(void *argument);

		BaseType_t taskHandle;

		HardwareLayer::AdcDriver& adc;
		Common::SystemData& systemData;
		Drv8316rSpiDriver& drv;

		float currentGain;
		float phaseOffsets[3] = {0.0, 0.0, 0.0};
		LowPassFilter phaseOffsetFilter[3] = { LowPassFilter(0.01f), LowPassFilter(0.01f), LowPassFilter(0.01f) };
		LowPassFilter phaseCurrentFilter[3] = { LowPassFilter(0.1f), LowPassFilter(0.1f), LowPassFilter(0.1f) };
		bool isCalibrated = false;
	};
}
#endif //ANALOG_PROCESSOR_HPP
