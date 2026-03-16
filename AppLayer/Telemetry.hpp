#ifndef TELEMETRY_HPP
#define TELEMETRY_HPP

#include "SystemData.hpp"
#include "AnalogProcessor.hpp"
#include "IEncoder.hpp"

namespace AppLayer
{

class Telemetry
{
public:
	Telemetry(Common::SystemData &systemDataRef, AnalogProcessor &analogProcessorRef,
			 HardwareLayer::IEncoder &rotorEncoderRef);
	void Init();

private:
	static void TelemetryTask(void *argument);

	Common::SystemData &systemData;
	AnalogProcessor &analogProcessor;
	HardwareLayer::IEncoder &rotorEncoder;
};

} // namespace AppLayer

#endif // TELEMETRY_HPP
