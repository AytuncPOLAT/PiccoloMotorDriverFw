#include "DRV8316R_SpiDriver.hpp"
#include "DebugPrint.hpp"

Drv8316rSpiDriver::Drv8316rSpiDriver(HardwareLayer::SpiBus& spiBusRef)
: spiBus(spiBusRef)
{
}

void Drv8316rSpiDriver::Init()
{
	spiBus.Init();
}

bool Drv8316rSpiDriver::Configure()
{
	SendCommand(0, 0);
	SetCurr2();
	SetOCP();
	return true;
}

std::uint8_t Drv8316rSpiDriver::SendCommand(std::uint8_t address,
		std::uint8_t data)
{
	drv8316_write_reg(0x03, 0x03);
	return 0;
}

void Drv8316rSpiDriver::SetCurr1()
{
	drv8316_write_reg(0x07, 0x00);
}

void Drv8316rSpiDriver::SetCurr2()
{
	drv8316_write_reg(0x07, 0x03);
}

void Drv8316rSpiDriver::SetOCP()
{
	drv8316_write_reg(0x06, 0x05);
}

bool Drv8316rSpiDriver::ClearFaults()
{
	drv8316_write_reg(0x04, 0x61);
	return true;
}

void Drv8316rSpiDriver::drv8316_write_reg(uint8_t reg, uint8_t data)
{
	uint16_t out = 0;
	out |= (data & 0xFF);
	out |= (reg & 0x3F) << 9;

	uint16_t parity = out;
	parity ^= parity >> 8;
	parity ^= parity >> 4;
	parity ^= parity >> 2;
	parity ^= parity >> 1;
	out |= (parity & 1) << 8;
	uint16_t in = 0;
	spiBus.Transfer16(out, in);
	outputDataFrame.data = in & 0xFF;
	outputDataFrame.status = (in >> 8) & 0xFF;
}
