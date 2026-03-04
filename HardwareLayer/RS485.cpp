#include "RS485.hpp"

using namespace HardwareLayer;

namespace
{
	Rs485* global_rs485;
}

extern "C"
{
	void UART4_IRQHandler(void)
	{
		HAL_UART_IRQHandler(&global_rs485->huart4);
	}

	void DMA1_Stream1_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(&global_rs485->hdma_uart4_rx);
	}

	void DMA1_Stream2_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(&global_rs485->hdma_uart4_tx);
	}

	void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
	{
		if(huart-> Instance == UART4)
		{
			global_rs485->txDoneFlag = true;
		}
	}

	void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
	{
		if (huart->Instance == UART4)
		{
			HAL_UARTEx_ReceiveToIdle_DMA(&global_rs485->huart4, global_rs485->rxBuffer, 20);
			if(global_rs485->callbackHandle != NULL)
				global_rs485->callbackHandle->OnReceiveCallback(global_rs485->rxBuffer, Size, global_rs485);
		}
	}
}

Rs485::Rs485()
: callbackHandle(nullptr)
{
	global_rs485 = this;
}

void* Rs485::GetInstance()
{
	return this;
}

void Rs485::Init()
{
	__HAL_RCC_DMA1_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA1_Stream0_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
	/* DMA1_Stream1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);

	//__HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE); // Enable RX interrupt
	//__HAL_UART_ENABLE_IT(&huart4, UART_IT_TXE); // Enable TX interrupt

	huart4.Instance = UART4;
	huart4.Init.BaudRate = 115200;
	huart4.Init.WordLength = UART_WORDLENGTH_8B;
	huart4.Init.StopBits = UART_STOPBITS_1;
	huart4.Init.Parity = UART_PARITY_NONE;
	huart4.Init.Mode = UART_MODE_TX_RX;
	huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart4.Init.OverSampling = UART_OVERSAMPLING_16;
	huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	HAL_RS485Ex_Init(&huart4, UART_DE_POLARITY_HIGH, 0, 0);

	HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8);

	HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8);

	HAL_UARTEx_DisableFifoMode(&huart4);

	HAL_UARTEx_ReceiveToIdle_DMA(&huart4, rxBuffer, 20);
}

extern "C"
{
	void HAL_UART_MspInit(UART_HandleTypeDef* huart)
	{
		GPIO_InitTypeDef GPIO_InitStruct = {0};
		RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

		if(huart->Instance==UART4)
		{
			PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART4;
			PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;

			HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

			/* Peripheral clock enable */
			__HAL_RCC_UART4_CLK_ENABLE();

			__HAL_RCC_GPIOA_CLK_ENABLE();
			__HAL_RCC_GPIOC_CLK_ENABLE();
			/**UART4 GPIO Configuration
			PA15(JTDI)     ------> UART4_DE
			PC11     ------> UART4_RX
			PC10     ------> UART4_TX
			*/
			GPIO_InitStruct.Pin = GPIO_PIN_15;
			GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
			HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

			GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_10;
			GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			GPIO_InitStruct.Alternate = GPIO_AF8_UART4;
			HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

			/* UART4 DMA Init */
			/* UART4_RX Init */
			global_rs485->hdma_uart4_rx.Instance = DMA1_Stream1;
			global_rs485->hdma_uart4_rx.Init.Request = DMA_REQUEST_UART4_RX;
			global_rs485->hdma_uart4_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
			global_rs485->hdma_uart4_rx.Init.PeriphInc = DMA_PINC_DISABLE;
			global_rs485->hdma_uart4_rx.Init.MemInc = DMA_MINC_ENABLE;
			global_rs485->hdma_uart4_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
			global_rs485->hdma_uart4_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
			global_rs485->hdma_uart4_rx.Init.Mode = DMA_NORMAL;
			global_rs485->hdma_uart4_rx.Init.Priority = DMA_PRIORITY_LOW;
			global_rs485->hdma_uart4_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
			HAL_DMA_Init(&global_rs485->hdma_uart4_rx);

			__HAL_LINKDMA(huart,hdmarx,global_rs485->hdma_uart4_rx);

			/* UART4_TX Init */
			global_rs485->hdma_uart4_tx.Instance = DMA1_Stream2;
			global_rs485->hdma_uart4_tx.Init.Request = DMA_REQUEST_UART4_TX;
			global_rs485->hdma_uart4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
			global_rs485->hdma_uart4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
			global_rs485->hdma_uart4_tx.Init.MemInc = DMA_MINC_ENABLE;
			global_rs485->hdma_uart4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
			global_rs485->hdma_uart4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
			global_rs485->hdma_uart4_tx.Init.Mode = DMA_NORMAL;
			global_rs485->hdma_uart4_tx.Init.Priority = DMA_PRIORITY_LOW;
			global_rs485->hdma_uart4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
			HAL_DMA_Init(&global_rs485->hdma_uart4_tx);

			__HAL_LINKDMA(huart,hdmatx,global_rs485->hdma_uart4_tx);

			/* UART4 interrupt Init */
			HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
			HAL_NVIC_EnableIRQ(UART4_IRQn);
		}
	}
}

uint8_t Rs485::Receive(uint8_t *data, uint32_t size)
{
	return 0;
}

uint8_t Rs485::Transmit(uint8_t *data, uint32_t size)
{
	if (txDoneFlag == true)
	{
		HAL_UART_Transmit_DMA(&huart4, data, size);
		txDoneFlag = false;
		return 0;
	}

	return 1;
}

void Rs485::RegisterOnReceiveCallback(Common::IUart::Callback* callback)
{
	callbackHandle = callback;
}

