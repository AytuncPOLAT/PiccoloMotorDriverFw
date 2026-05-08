#include "AS5047_Driver.hpp"

using namespace HardwareLayer;

namespace
{
	constexpr uint16_t ADDR_ANGLECOM = 0x3FFF;
	AS5047* as5047ptr;
}

extern "C"
{
	void SPI3_IRQHandler(void)
	{
		HAL_SPI_IRQHandler(&as5047ptr->spiHandle);
	}
	void DMA1_Stream3_IRQHandler(void)   // RX complete
	{
		HAL_DMA_IRQHandler(&as5047ptr->hdma_spi3_rx);
	}

	void DMA1_Stream4_IRQHandler(void)   // TX complete
	{
		HAL_DMA_IRQHandler(&as5047ptr->hdma_spi3_tx);
	}
	void HAL_SPI_TxRxHalfCpltCallback(SPI_HandleTypeDef *hspi)
	{
		if (hspi->Instance == SPI3)
		{
			//as5047ptr->OnTransferComplete();
		}
	}

	void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
	{
	    if (hspi->Instance == SPI3)
	    {
	    	as5047ptr->OnTransferComplete();
	    }
	}
}



AS5047::AS5047()
: speedFilter(0.01f)
{
	as5047ptr = this;
}

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
	spiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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

	spiHandle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
	if (HAL_SPI_Init(&spiHandle) != HAL_OK)
	{
		//Error_Handler();
	}

	DMA_Init();
}


void AS5047::DMA_Init()
{
	// ── DMA clocks ─────────────────────────────────────────────────────────────
	__HAL_RCC_DMA1_CLK_ENABLE();
	//__HAL_RCC_DMAMUX1_CLK_ENABLE();   // Required on H7 — often forgotten!

	// ── RX: DMA1 Stream0, DMAMUX1 request 61 (SPI3_RX) ────────────────────────
	hdma_spi3_rx.Instance                 = DMA1_Stream3;
	hdma_spi3_rx.Init.Request             = DMA_REQUEST_SPI3_RX;      // 61
	hdma_spi3_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	hdma_spi3_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_spi3_rx.Init.MemInc              = DMA_MINC_DISABLE;         // single word
	hdma_spi3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;  // 16-bit SPI
	hdma_spi3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
	hdma_spi3_rx.Init.Mode                = DMA_NORMAL;
	hdma_spi3_rx.Init.Priority            = DMA_PRIORITY_HIGH;
	hdma_spi3_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;     // required for 16-bit
	HAL_DMA_Init(&hdma_spi3_rx) != HAL_OK;
	__HAL_LINKDMA(&spiHandle, hdmarx, hdma_spi3_rx);

	// ── TX: DMA1 Stream1, DMAMUX1 request 62 (SPI3_TX) ────────────────────────
	hdma_spi3_tx.Instance                 = DMA1_Stream4;
	hdma_spi3_tx.Init.Request             = DMA_REQUEST_SPI3_TX;      // 62
	hdma_spi3_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	hdma_spi3_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_spi3_tx.Init.MemInc              = DMA_MINC_DISABLE;         // single word
	hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_spi3_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
	hdma_spi3_tx.Init.Mode                = DMA_NORMAL;
	hdma_spi3_tx.Init.Priority            = DMA_PRIORITY_LOW;
	hdma_spi3_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
	HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK;
	__HAL_LINKDMA(&spiHandle, hdmatx, hdma_spi3_tx);

	// ── NVIC ───────────────────────────────────────────────────────────────────
	// Only RX interrupt needed — it fires last, signalling full duplex done
	HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
	// TX interrupt optional, enable if you need it
	HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 6, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
	// SPI3 interrupt
	HAL_NVIC_SetPriority(SPI3_IRQn, 5,0);
	HAL_NVIC_EnableIRQ(SPI3_IRQn);
}

void AS5047::StartAsyncRead()
{
    //uint16_t cmd = ADDR_ANGLECOM | 0x4000 | 0x8000; // read + parity bit set
	uint16_t rxWord;
	uint16_t cmd = 0;
	uint16_t sumOfOnes = 0;
	cmd = ADDR_ANGLECOM;
	cmd |= 0x4000; //Read cmd

	/*
	for(uint8_t i = 0; i <= 14; i++)
	{
		uint16_t bitState = (cmd & (1U << i)) >> i;
		sumOfOnes += bitState;
	}

	cmd |= (~(sumOfOnes%2)) << 15;
 	*/

	cmd |= 0x8000;

    dmaTxBuf = cmd;
    dmaTransferDone = false;
    HAL_SPI_TransmitReceive_DMA(&spiHandle,
                                 (uint8_t*)&dmaTxBuf,
                                 (uint8_t*)&dmaRxBuf,
                                 1);  // 1 = one 16-bit frame (set by DataSize)
}

void AS5047::OnTransferComplete()
{
    position = (int)(dmaRxBuf & 0x3FFF);
    dmaTransferDone = true;
}

int AS5047::GetPosition_Async()
{
    return position;   // always returns last DMA-settled value
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
	//position = (int)SPI_Read(ADDR_ANGLECOM);
	//return position;
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

	//return (int)speedFilter.Update((float)speed);
	return speed;

}

void AS5047::RegisterOnIndexPulseCallback(Callback* callback)
{

}
