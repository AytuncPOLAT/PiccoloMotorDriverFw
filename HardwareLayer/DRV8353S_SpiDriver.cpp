#include "DRV8353S_SpiDriver.hpp"

using namespace HardwareLayer;

constexpr Drv8353sSpiDriver::Config Drv8353sSpiDriver::DEFAULT_CONFIG;

namespace
{
	constexpr std::uint16_t READ_BIT = 1u << 15;
	constexpr std::uint16_t ADDRESS_SHIFT = 11;

	// Driver Control (0x02)
	constexpr std::uint16_t CLR_FLT = 1u << 0;

	// Gate Drive HS (0x03) / Gate Drive LS (0x04)
	constexpr std::uint16_t IDRIVEP_SHIFT = 4;
	constexpr std::uint16_t IDRIVEP_MASK = 0x0Fu << IDRIVEP_SHIFT;
	constexpr std::uint16_t IDRIVEN_SHIFT = 0;
	constexpr std::uint16_t IDRIVEN_MASK = 0x0Fu << IDRIVEN_SHIFT;
	constexpr std::uint16_t TDRIVE_SHIFT = 8;
	constexpr std::uint16_t TDRIVE_MASK = 0x03u << TDRIVE_SHIFT;

	// CSA Control (0x06)
	constexpr std::uint16_t CSA_GAIN_SHIFT = 6;
	constexpr std::uint16_t CSA_GAIN_MASK = 0x03u << CSA_GAIN_SHIFT;
}

Drv8353sSpiDriver::Drv8353sSpiDriver(SpiBus& spiBusRef,
									 GPIO_TypeDef* enablePortRef,
									 std::uint16_t enablePinRef,
									 const Config& initialConfig)
: spiBus(spiBusRef)
, enablePort(enablePortRef)
, enablePin(enablePinRef)
, config(initialConfig)
{
}

void Drv8353sSpiDriver::Init()
{
	spiBus.Init();
	SetEnabled(true);
}

void Drv8353sSpiDriver::SetEnabled(bool enabled)
{
	HAL_GPIO_WritePin(enablePort, enablePin, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);

	if(enabled && !isEnabled)
		HAL_Delay(WAKE_TIME_MS);

	isEnabled = enabled;
}

bool Drv8353sSpiDriver::Configure()
{
	bool ok = ClearFaults();
	ok &= SetCsaGain(config.csaGain);
	ok &= SetGateDriveStrength(config.gateDrive);
	return ok;
}

bool Drv8353sSpiDriver::SetCsaGain(CsaGain gain)
{
	config.csaGain = gain;
	return ModifyRegister(Register::CSA_CONTROL, CSA_GAIN_MASK,
						  static_cast<std::uint16_t>(gain) << CSA_GAIN_SHIFT);
}

float Drv8353sSpiDriver::GetCsaGainVoltsPerVolt() const
{
	switch(config.csaGain)
	{
	case CsaGain::GAIN_5_VV:  return 5.0f;
	case CsaGain::GAIN_10_VV: return 10.0f;
	case CsaGain::GAIN_20_VV: return 20.0f;
	case CsaGain::GAIN_40_VV: return 40.0f;
	default:                  return 0.0f;
	}
}

bool Drv8353sSpiDriver::SetGateDriveStrength(const GateDriveStrength& strength)
{
	bool ok = SetHighSideGateDrive(strength.highSideSource, strength.highSideSink);
	ok &= SetLowSideGateDrive(strength.lowSideSource, strength.lowSideSink, strength.tdrive);
	return ok;
}

bool Drv8353sSpiDriver::SetHighSideGateDrive(IdriveSource source, IdriveSink sink)
{
	config.gateDrive.highSideSource = source;
	config.gateDrive.highSideSink = sink;

	// LOCK bits (10..8) are preserved by the read-modify-write.
	const std::uint16_t value = (static_cast<std::uint16_t>(source) << IDRIVEP_SHIFT) |
								(static_cast<std::uint16_t>(sink) << IDRIVEN_SHIFT);
	return ModifyRegister(Register::GATE_DRIVE_HS, IDRIVEP_MASK | IDRIVEN_MASK, value);
}

bool Drv8353sSpiDriver::SetLowSideGateDrive(IdriveSource source, IdriveSink sink, Tdrive tdrive)
{
	config.gateDrive.lowSideSource = source;
	config.gateDrive.lowSideSink = sink;
	config.gateDrive.tdrive = tdrive;

	const std::uint16_t value = (static_cast<std::uint16_t>(tdrive) << TDRIVE_SHIFT) |
								(static_cast<std::uint16_t>(source) << IDRIVEP_SHIFT) |
								(static_cast<std::uint16_t>(sink) << IDRIVEN_SHIFT);
	return ModifyRegister(Register::GATE_DRIVE_LS, TDRIVE_MASK | IDRIVEP_MASK | IDRIVEN_MASK, value);
}

bool Drv8353sSpiDriver::ClearFaults()
{
	std::uint16_t driverControl = 0;
	if(!ReadRegister(Register::DRIVER_CONTROL, driverControl))
		return false;

	// CLR_FLT self-clears, so no read-back verification here.
	return WriteRegister(Register::DRIVER_CONTROL, driverControl | CLR_FLT);
}

Drv8353sSpiDriver::FaultStatus Drv8353sSpiDriver::ReadFaultStatus()
{
	FaultStatus status {0, 0};
	ReadRegister(Register::FAULT_STATUS_1, status.faultStatus1);
	ReadRegister(Register::VGS_STATUS_2, status.vgsStatus2);
	return status;
}

bool Drv8353sSpiDriver::ReadRegister(Register reg, std::uint16_t& value)
{
	const std::uint16_t txWord = READ_BIT |
		((static_cast<std::uint16_t>(reg) & 0x0F) << ADDRESS_SHIFT);
	std::uint16_t rxWord = 0;

	if(!spiBus.Transfer16(txWord, rxWord))
		return false;

	value = rxWord & DATA_MASK;
	return true;
}

bool Drv8353sSpiDriver::WriteRegister(Register reg, std::uint16_t value)
{
	const std::uint16_t txWord =
		((static_cast<std::uint16_t>(reg) & 0x0F) << ADDRESS_SHIFT) | (value & DATA_MASK);
	std::uint16_t rxWord = 0;

	return spiBus.Transfer16(txWord, rxWord);
}

bool Drv8353sSpiDriver::ModifyRegister(Register reg, std::uint16_t mask, std::uint16_t value)
{
	std::uint16_t current = 0;
	if(!ReadRegister(reg, current))
		return false;

	const std::uint16_t updated = (current & ~mask) | (value & mask);
	if(!WriteRegister(reg, updated))
		return false;

	std::uint16_t readBack = 0;
	if(!ReadRegister(reg, readBack))
		return false;

	return (readBack & mask) == (value & mask);
}
