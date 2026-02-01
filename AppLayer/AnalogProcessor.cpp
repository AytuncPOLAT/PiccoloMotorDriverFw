#include "AnalogProcessor.hpp"

//volatile int GLOBAL_ADC_0;
volatile int GLOBAL_ADC_1;
volatile int GLOBAL_ADC_2;
uint16_t GLOBAL_ADC_3;
uint16_t GLOBAL_ADC_4;
uint16_t GLOBAL_ADC_5;
//uint16_t GLOBAL_ADC_6;

using namespace AppLayer;

AnalogProcessor::AnalogProcessor(HardwareLayer::AdcDriver& adcRef, Common::SystemData& systemDataRef)
: adc(adcRef)
, systemData(systemDataRef)
{
}

void AnalogProcessor::AnalogProcessTask(void *argument)
{
	AnalogProcessor *objectHandle = static_cast<AnalogProcessor*>(argument);
	while(1)
	{
		//vTaskDelete(NULL);
		//GLOBAL_ADC_0 = objectHandle->GetPhaseCurrent(0);
		GLOBAL_ADC_1 = objectHandle->GetPhaseCurrent(1);
		GLOBAL_ADC_2 = objectHandle->GetPhaseCurrent(2);
		GLOBAL_ADC_3 = objectHandle->adc.ReadChannel(3);

		osDelay(1);
	}
}

float AnalogProcessor::GetBusVoltage()
{
	float busVoltage = ((adc.ReadChannel(Common::ADC_CHANNELS::DC_BUS_VOLTAGE) -72) * Common::MILLIVOLTS_PER_COUNT) / Common::DC_BUS_SENSE_RATIO;
	return busVoltage;
}

float AnalogProcessor::GetPhaseCurrent(uint8_t channel)
{
	float currentInAmps = adc.ReadChannel(channel) * Common::MILLIVOLTS_PER_COUNT;
	return currentInAmps;
}

void AnalogProcessor::SetVoltageCurrentRatio(float gain)
{
	currentGain = gain;
}

void AnalogProcessor::StartTask()
{
	taskHandle = xTaskCreate(this->AnalogProcessTask,
			"AnalogProcessTask",
			128 * 4,
			(void*) this,
			24,
			NULL);
}
