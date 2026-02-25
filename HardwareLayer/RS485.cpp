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
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == UART4)
	{
		//HAL_UART_Receive_IT(&global_rs485->huart4, global_rs485->rxBuffer, Size);
		global_rs485->callbackHandle->OnReceiveCallback(global_rs485->rxBuffer, Size);
	}
}

Rs485::Rs485()
: callbackHandle(nullptr)
{
	global_rs485 = this;
}

void Rs485::Init()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART4;
	PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		//Error_Handler();
	}

	__HAL_RCC_UART4_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();

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
	if (HAL_RS485Ex_Init(&huart4, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		//Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		//Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		//Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)
	{
		//Error_Handler();
	}

	//__HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE); // Enable RX interrupt
	//__HAL_UART_ENABLE_IT(&huart4, UART_IT_TXE); // Enable TX interrupt

    //HAL_NVIC_SetPriority(UART4_IRQn, 0, 1);
    //HAL_NVIC_EnableIRQ(UART4_IRQn);

    HAL_UART_Receive_DMA(&global_rs485->huart4, global_rs485->rxBuffer, 4);
}

uint8_t Rs485::Receive(uint8_t *data, uint32_t size)
{

}

uint8_t Rs485::Transmit(uint8_t *data, uint32_t size)
{
	HAL_UART_Transmit(&huart4, data, size, 100);
	return 0;
}

void Rs485::RegisterOnReceiveCallback(Common::IUart::Callback* callback)
{
	callbackHandle = callback;
}

