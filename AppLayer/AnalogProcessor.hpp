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
		uint16_t GetExtAnalog(uint8_t channel);
		bool IsCalibrated();
		void SetVoltageCurrentRatio(float gain);
		void StartTask();
		void ResetConversionDoneFlag();
		bool GetConversionDoneFlag();


	private:
		static void AnalogProcessTask(void *argument);

		BaseType_t taskHandle;

		HardwareLayer::AdcDriver& adc;
		Common::SystemData& systemData;
		float currentGain;
		float phaseOffsets[3] = {0.0, 0.0, 0.0};

		bool isCalibrated = false;
	};
}
#endif //ANALOG_PROCESSOR_HPP
