#include "AnalogProcessor.hpp"
#include "BoardConfig/BoardConfig.h"

using namespace AppLayer;

namespace
{
	static_assert(BOARD_SHUNT_RESISTOR_OHMS > 0.0f,
		"Board shunt resistance must be positive");
	static_assert(BOARD_SUPPLY_DIVIDER_LOW_SIDE_OHMS > 0U &&
		BOARD_SUPPLY_DIVIDER_HIGH_SIDE_OHMS > 0U,
		"Board supply divider resistances must be positive");
}

AnalogProcessor::AnalogProcessor(HardwareLayer::AdcDriver& adcRef,
		Common::SystemData& systemDataRef,
		HardwareLayer::GateDriver& drvRef)
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
	constexpr float busSenseRatio =
		static_cast<float>(BOARD_SUPPLY_DIVIDER_LOW_SIDE_OHMS) /
		(BOARD_SUPPLY_DIVIDER_LOW_SIDE_OHMS + BOARD_SUPPLY_DIVIDER_HIGH_SIDE_OHMS);
	float busVoltage = (adc.ReadChannel(Common::ADC_CHANNELS::DC_BUS_VOLTAGE) * Common::MILLIVOLTS_PER_COUNT)
			/ busSenseRatio;
	return busVoltage;
}

float AnalogProcessor::GetPhaseCurrent(uint8_t channel)
{
	float currentInAmps = 0.0;
	float ampsPerCount = (Common::MILLIVOLTS_PER_COUNT /
		(BOARD_SHUNT_RESISTOR_OHMS * 20.0f)) / 1000.0;
#if defined(GATE_DRIVER_DRV8353S)
	// Preserve the existing 20 V/V calibration when the hardware gain changes.
	ampsPerCount *= 20.0f / drv.GetCsaGainVoltsPerVolt();
#endif

	switch (channel)
	{
	case 0:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_A_CURRENT) - phaseOffsets[channel]) * ampsPerCount;
		currentInAmps = phaseCurrentFilter[0].Update(currentInAmps);
		break;

	case 1:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_B_CURRENT) - phaseOffsets[channel]) * ampsPerCount;
		currentInAmps = phaseCurrentFilter[1].Update(currentInAmps);
		break;

	case 2:
		currentInAmps = (adc.ReadChannel(Common::ADC_CHANNELS::PHASE_C_CURRENT) - phaseOffsets[channel]) * ampsPerCount;
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
