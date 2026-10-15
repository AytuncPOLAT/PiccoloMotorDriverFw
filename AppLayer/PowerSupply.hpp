#ifndef POWER_SUPPLY_HPP
#define POWER_SUPPLY_HPP

#include <stdint.h>

extern volatile float GLOBAL_BOOT_BUS_VOLTAGE;
extern volatile bool GLOBAL_UNDERVOLTAGE_ERROR;

namespace AppLayer
{
class AnalogProcessor;
class SystemError;

class PowerSupply
{
public:
	PowerSupply(AnalogProcessor& analogProcessorRef, SystemError& systemErrorRef);

	void Init();
	void SampleBootVoltage();
	void Update();

	float GetBusVoltage() const;
	float GetBootBusVoltage() const;
	bool IsUndervoltage() const;
	bool IsBootVoltageSampled() const;

private:
	AnalogProcessor& analogProcessor;
	SystemError& systemError;

	float bootBusVoltage = 0.0f;
	float currentBusVoltage = 0.0f;
	bool isBootVoltageSampled = false;
	bool isUndervoltage = false;
	uint8_t debounceCounter = 0;

	static constexpr float UNDERVOLTAGE_DROP_THRESHOLD_MV = 1000.0f; // 1.0 Volt in millivolts
	static constexpr uint8_t DEBOUNCE_THRESHOLD = 5;                  // 5 consecutive 10ms checks (50ms)
};

} // namespace AppLayer

#endif // POWER_SUPPLY_HPP

