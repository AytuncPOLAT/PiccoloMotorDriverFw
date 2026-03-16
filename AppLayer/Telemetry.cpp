#include "Telemetry.hpp"

using namespace AppLayer;

Telemetry::Telemetry(Common::SystemData &systemDataRef, AnalogProcessor &analogProcessorRef,
			 HardwareLayer::IEncoder &rotorEncoderRef)
: systemData(systemDataRef)
, analogProcessor(analogProcessorRef)
, rotorEncoder(rotorEncoderRef)
{
}

void Telemetry::Init()
{
	xTaskCreate(TelemetryTask,
			"TelemetryTask",
			128 * 4,
			(void*)this,
			25,
			NULL);
}

void Telemetry::TelemetryTask(void *argument)
{
	Telemetry *objectHandle = static_cast<Telemetry*>(argument);

	while(1)
	{
		float busVoltage = objectHandle->analogProcessor.GetBusVoltage();
		objectHandle->systemData.realtimeData.dcBusVoltage = (int)busVoltage;
		objectHandle->systemData.realtimeData.multiTurnEncoder = objectHandle->rotorEncoder.GetPosition();
		osDelay(100);
	}
}
