#include "QuadraticEncoderDriver.hpp"
#include "stm32h7xx_hal.h"

using namespace HardwareLayer;

QuadraticEncoderDriver::QuadraticEncoderDriver()
{

}

void QuadraticEncoderDriver::Init()
{
	TIM_HandleTypeDef htim5;
	TIM_Encoder_InitTypeDef sConfig = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	__HAL_RCC_TIM5_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	/**TIM5 GPIO Configuration
	PA0     ------> TIM5_CH1
	PA1     ------> TIM5_CH2
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	htim5.Instance = TIM5;
	htim5.Init.Prescaler = 0;
	htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim5.Init.Period = 0x800;
	htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
	sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC1Filter = 0;
	sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
	sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
	sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
	sConfig.IC2Filter = 0;
	if (HAL_TIM_Encoder_Init(&htim5, &sConfig) != HAL_OK)
	{
	//Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
	{
	//Error_Handler();
	}

	HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);

}

int QuadraticEncoderDriver::GetPosition()
{
	return TIM5->CNT << 1;
}

int QuadraticEncoderDriver::GetSpeed()
{
	return 0;
}

void QuadraticEncoderDriver::Reset()
{

}

void QuadraticEncoderDriver::RegisterOnIndexPulseCallback(Callback* callback)
{

}
