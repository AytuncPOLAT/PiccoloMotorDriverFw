#include "SpiBus.hpp"

using namespace HardwareLayer;

void SpiBus::Init()
{
	if(isInitialized)
		return;

	hspi.Instance = SPI2;
	hspi.Init.Mode = SPI_MODE_MASTER;
	hspi.Init.Direction = SPI_DIRECTION_2LINES;
	hspi.Init.DataSize = SPI_DATASIZE_16BIT;
	hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
	hspi.Init.NSS = SPI_NSS_HARD_OUTPUT;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial = 0x0;
	hspi.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	hspi.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
	hspi.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
	hspi.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	hspi.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
	hspi.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
	hspi.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
	hspi.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
	hspi.Init.IOSwap = SPI_IO_SWAP_DISABLE;

	if(HAL_SPI_Init(&hspi) == HAL_OK)
		isInitialized = true;
}

bool SpiBus::Transfer16(std::uint16_t txWord, std::uint16_t& rxWord)
{
	if(!isInitialized)
		return false;

	return HAL_SPI_TransmitReceive(&hspi,
								   reinterpret_cast<uint8_t*>(&txWord),
								   reinterpret_cast<uint8_t*>(&rxWord),
								   1, TIMEOUT_MS) == HAL_OK;
}
