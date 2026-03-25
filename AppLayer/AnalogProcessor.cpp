#include "AnalogProcessor.hpp"

using namespace AppLayer;

AnalogProcessor::AnalogProcessor(HardwareLayer::AdcDriver& adcRef,
		Common::SystemData& systemDataRef,
		Drv8316rSpiDriver& drvRef)
: adc(adcRef)
, systemData(systemDataRef)
, drv(drvRef)
{
}

void AnalogProcessor::Init()
{
	taskHandle = xTaskCreate(this->AnalogProcessTask,
			"AnalogProcessTask",
			128 * 4,
			(void*) this,
			24,
			NULL);
}

void AnalogProcessor::ResetConversionDoneFlag()
{
	adc.ResetConversionDoneFlag();
}

bool AnalogProcessor::GetConversionDoneFlag()
{
	return adc.isConversionDone;
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

		objectHandle->phaseOffsets[0] = objectHandle->phaseOffsetFilter[0].Update(
			objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::PHASE_A_CURRENT));

		objectHandle->phaseOffsets[1] = objectHandle->phaseOffsetFilter[1].Update(
			objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::PHASE_B_CURRENT));

		objectHandle->phaseOffsets[2] = objectHandle->phaseOffsetFilter[2].Update(
			objectHandle->adc.ReadChannel(Common::ADC_CHANNELS::PHASE_C_CURRENT));

		osDelay(1);
	}

	objectHandle->isCalibrated = true;
	vTaskDelete(NULL);
}

bool AnalogProcessor::IsCalibrated()
{
	return isCalibrated;
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
		currentInAmps = phaseCurrentFilter[0].Update(currentInAmps);
		break;

	case 1:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_B_CURRENT) - phaseOffsets[channel]) * Common::MILLIAMPS_PER_COUNT;
		currentInAmps = phaseCurrentFilter[1].Update(currentInAmps);
		break;

	case 2:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_C_CURRENT) - phaseOffsets[channel]) * Common::MILLIAMPS_PER_COUNT;
		currentInAmps = phaseCurrentFilter[2].Update(currentInAmps);
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

void AnalogProcessor::SetCurrentSenseGain()
{

}

