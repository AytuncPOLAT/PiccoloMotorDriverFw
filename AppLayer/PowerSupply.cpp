#include "PowerSupply.hpp"
#include "AnalogProcessor.hpp"
#include "SystemError.hpp"
#include "cmsis_os.h"

volatile float GLOBAL_BOOT_BUS_VOLTAGE = 0.0f;
volatile bool GLOBAL_UNDERVOLTAGE_ERROR = false;

using namespace AppLayer;

PowerSupply::PowerSupply(AnalogProcessor& analogProcessorRef, SystemError& systemErrorRef)
	: analogProcessor(analogProcessorRef)
	, systemError(systemErrorRef)
	, bootBusVoltage(0.0f)
	, currentBusVoltage(0.0f)
	, isBootVoltageSampled(false)
	, isUndervoltage(false)
	, debounceCounter(0)
{
}

void PowerSupply::Init()
{
	SampleBootVoltage();
}

void PowerSupply::SampleBootVoltage()
{
	// Wait for ADC and DMA conversions to stabilize
	osDelay(100);

	const uint8_t sampleCount = 20;
	float voltageSum = 0.0f;

	for (uint8_t i = 0; i < sampleCount; ++i)
	{
		voltageSum += analogProcessor.GetBusVoltage();
		osDelay(5);
	}

	bootBusVoltage = voltageSum / (float)sampleCount;
	currentBusVoltage = bootBusVoltage;
	isBootVoltageSampled = true;

	GLOBAL_BOOT_BUS_VOLTAGE = bootBusVoltage;
	GLOBAL_UNDERVOLTAGE_ERROR = false;
}

void PowerSupply::Update()
{
	if (!isBootVoltageSampled)
	{
		SampleBootVoltage();
		return;
	}

	currentBusVoltage = analogProcessor.GetBusVoltage();

	if (!isUndervoltage)
	{
		// Check if supply voltage dropped by 1.0 V (1000 mV) relative to boot voltage
		if ((bootBusVoltage - currentBusVoltage) >= UNDERVOLTAGE_DROP_THRESHOLD_MV)
		{
			debounceCounter++;
			if (debounceCounter >= DEBOUNCE_THRESHOLD)
			{
				isUndervoltage = true;
				GLOBAL_UNDERVOLTAGE_ERROR = true;
				systemError.SetError(Common::ErrorType::UNDER_VOLTAGE);
			}
		}
		else
		{
			debounceCounter = 0;
		}
	}
	else
	{
		// Maintain error state while undervoltage condition persisted
		GLOBAL_UNDERVOLTAGE_ERROR = true;
	}
}

float PowerSupply::GetBusVoltage() const
{
	return currentBusVoltage;
}

float PowerSupply::GetBootBusVoltage() const
{
	return bootBusVoltage;
}

bool PowerSupply::IsUndervoltage() const
{
	return isUndervoltage;
}

bool PowerSupply::IsBootVoltageSampled() const
{
	return isBootVoltageSampled;
}

