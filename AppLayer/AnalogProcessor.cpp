#include "AnalogProcessor.hpp"

//volatile int GLOBAL_ADC_0;
float G_PHASE_CURRENT_A;
float G_PHASE_CURRENT_B;
float G_PHASE_CURRENT_C;
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

	objectHandle->phaseOffsets[0] = Common::ADC_MID_POINT;
	objectHandle->phaseOffsets[1] = Common::ADC_MID_POINT;
	objectHandle->phaseOffsets[2] = Common::ADC_MID_POINT;

	uint16_t numberOfCalibCycles = 1000;
	while(1)
	{
		if(numberOfCalibCycles-- == 0)
			break;

		objectHandle->phaseOffsets[0] = objectHandle->phaseOffsets[0] * 0.99 +
				objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::PHASE_A_CURRENT) * 0.01;

		objectHandle->phaseOffsets[1] = objectHandle->phaseOffsets[1] * 0.99 +
				objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::PHASE_B_CURRENT) * 0.01;

		objectHandle->phaseOffsets[2] = objectHandle->phaseOffsets[2] * 0.99 +
				objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::PHASE_C_CURRENT) * 0.01;

		osDelay(10);
	}
	vTaskDelete(NULL);
}

float AnalogProcessor::GetBusVoltage()
{
	float busVoltage = (adc.ReadChannel(Common::ADC_CHANNELS::DC_BUS_VOLTAGE) * Common::MILLIVOLTS_PER_COUNT)
			/ Common::DC_BUS_SENSE_RATIO;
	return busVoltage;
}

float AnalogProcessor::GetPhaseCurrent(uint8_t channel)
{
	float currentInAmps = 0.0;

	switch (channel)
	{
	case 0:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_A_CURRENT) - phaseOffsets[channel]) * Common::MILLIAMPS_PER_COUNT;
		G_PHASE_CURRENT_A = currentInAmps;
		break;

	case 1:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_B_CURRENT) - phaseOffsets[channel]) * Common::MILLIAMPS_PER_COUNT;
		G_PHASE_CURRENT_B = currentInAmps;
		break;

	case 2:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_C_CURRENT) - phaseOffsets[channel]) * Common::MILLIAMPS_PER_COUNT;
		G_PHASE_CURRENT_C = currentInAmps;
		break;

	default:
		currentInAmps = 999.9;
		break;
	}

	return currentInAmps;
}

uint16_t AnalogProcessor::GetExtAnalog(uint8_t channel)
{
	uint16_t analogValue = 0;
	switch (channel)
	{
	case 0:
		analogValue = adc.ReadChannel(Common::ADC_CHANNELS::EXT0);
		break;

	case 1:
		analogValue = adc.ReadChannel(Common::ADC_CHANNELS::EXT1);
		break;
	default:
		analogValue = 0xFFFF;
		break;
	}

	return analogValue;
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
