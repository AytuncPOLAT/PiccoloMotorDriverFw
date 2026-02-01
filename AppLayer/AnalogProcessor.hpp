#ifndef ANALOG_PROCESSOR_HPP
#define ANALOG_PROCESSOR_HPP

extern "C"
{
//#include "main.h"
#include "cmsis_os.h"
}

#include "AdcDriver.hpp"
#include "SystemData.hpp"

namespace AppLayer
{
	class AnalogProcessor
	{
	public:
		AnalogProcessor(HardwareLayer::AdcDriver& adcRef,
				Common::SystemData& systemDataRef);

		float GetPhaseCurrent(uint8_t channel);
		float GetBusVoltage();
		void SetVoltageCurrentRatio(float gain);
		void StartTask();

	private:
		static void AnalogProcessTask(void *argument);

		BaseType_t taskHandle;

		HardwareLayer::AdcDriver& adc;
		Common::SystemData& systemData;
		float currentGain;

	};
}
#endif //ANALOG_PROCESSOR_HPP
