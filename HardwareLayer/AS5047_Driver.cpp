#include "AS5047_Driver.hpp"

using namespace HardwareLayer;

namespace
{
	constexpr uint16_t ADDR_ANGLECOM = 0x3FFF;
}

AS5047::AS5047()
: speedFilter(0.1f)
{}

void AS5047::Init()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	__HAL_RCC_SPI3_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI3;
	PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_CLKP;

    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	spiHandle.Instance = SPI3;
	spiHandle.Init.Mode = SPI_MODE_MASTER;
	spiHandle.Init.Direction = SPI_DIRECTION_2LINES;
	spiHandle.Init.DataSize = SPI_DATASIZE_16BIT;
	spiHandle.Init.CLKPolarity = SPI_POLARITY_LOW;
	spiHandle.Init.CLKPhase = SPI_PHASE_2EDGE;
	spiHandle.Init.NSS = SPI_NSS_HARD_OUTPUT;
	spiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	spiHandle.Init.FirstBit = SPI_FIRSTBIT_MSB;
	spiHandle.Init.TIMode = SPI_TIMODE_DISABLE;
	spiHandle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	spiHandle.Init.CRCPolynomial = 0x0;
	spiHandle.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
	spiHandle.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
	spiHandle.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
	spiHandle.Init.TxCRCInitializationPattern =
		SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	spiHandle.Init.RxCRCInitializationPattern =
		SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
	spiHandle.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
	spiHandle.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
	spiHandle.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
	spiHandle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
	spiHandle.Init.IOSwap = SPI_IO_SWAP_DISABLE;
	if (HAL_SPI_Init(&spiHandle) != HAL_OK)
	{
		//Error_Handler();
	}
}


uint16_t AS5047::SPI_Read(uint16_t address)
{
	//TODO

	uint16_t rxWord;
	uint16_t cmd = 0;
	uint16_t sumOfOnes = 0;
	cmd = address;
	cmd |= 0x4000; //Read cmd

	for(uint8_t i = 0; i <= 14; i++)
	{
		uint16_t bitState = (cmd & (1U << i)) >> i;
		sumOfOnes += bitState;
	}

	//cmd |= (~(sumOfOnes%2)) << 15;

	cmd |= 0x8000;

	HAL_SPI_TransmitReceive(&spiHandle, (uint8_t*)&cmd, (uint8_t*)&rxWord, 1, 1000);

	rxWord &= 0x3FFF;

	return rxWord;
}

int AS5047::GetPosition()
{
	position = (int)SPI_Read(ADDR_ANGLECOM);
	return position;
}

int AS5047::GetMultiTurnPosition()
{
	if(position - oldPosMultiTurn > 8000)
	{
		multiTurnRev = multiTurnRev - 1;
	}
	else if(oldPosMultiTurn - position > 8000)
	{
		multiTurnRev = multiTurnRev + 1;
	}

	oldPosMultiTurn = position;
	return multiTurnRev * 16384 + position;
}

float AS5047::GetRotorAngleInRadians()
{
	int angle = (position >> 2) + offset;
	angle = angle % 585;

	float angleInRadians = ((float)angle / 585.0) * 2.0 * M_PI;
	return angleInRadians;
}

void AS5047::SetRotorEncoderOffset(int16_t newOffset)
{
	offset = newOffset;
}

void AS5047::Reset()
{
	// No action required for this encoder implementation
}

int AS5047::GetSpeed()
{
	int16_t speed;
	int16_t reducedPos = (int16_t)(position << 2);

	if(reducedPos >= 0)
		speed = abs(reducedPos) - abs(oldPosition);
	else
		speed = abs(oldPosition) - abs(reducedPos);

	oldPosition = reducedPos;

	return (int)speedFilter.Update((float)speed);

}

void AS5047::RegisterOnIndexPulseCallback(Callback* callback)
{

}
