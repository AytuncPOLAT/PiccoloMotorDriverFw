#include "DRV8316R_SpiDriver.hpp"
#include "DebugPrint.hpp"

Drv8316rSpiDriver::Drv8316rSpiDriver()
{
}

void Drv8316rSpiDriver::Init()
{
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_16BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
	hspi2.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 0x0;
	hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
	hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
	hspi2.Init.TxCRCInitializationPattern =
		SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi2.Init.RxCRCInitializationPattern =
		SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
	hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
	hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
	hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
	hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
	HAL_SPI_Init(&hspi2);
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

void Drv8316rSpiDriver::ClearFaults()
{
	drv8316_write_reg(0x04, 0x61);
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
	HAL_SPI_TransmitReceive(&hspi2, (uint8_t*)&out, (uint8_t*)&outputDataFrame, 1, 1000);
}
