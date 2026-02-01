#include "MotorPwm.hpp"
#include "SystemData.hpp"

using namespace HardwareLayer;

MotorPwm::MotorPwm(TIM_HandleTypeDef& timerRef)
: timerHandle(timerRef)
{
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	TIM_OC_InitTypeDef sConfigOC = {0};
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

	timerHandle.Instance = TIM1;
	timerHandle.Init.Prescaler = 1;
	timerHandle.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED2;
	timerHandle.Init.Period = Common::MOTOR_PWM_MAX_CNT;
	timerHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	timerHandle.Init.RepetitionCounter = 0;
	timerHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&timerHandle) != HAL_OK)
	{
		//Error_Handler();
	}
	if (HAL_TIM_OC_Init(&timerHandle) != HAL_OK)
	{
		//Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
	sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&timerHandle, &sMasterConfig) != HAL_OK)
	{
		//Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&timerHandle, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		//Error_Handler();
	}
	if (HAL_TIM_OC_ConfigChannel(&timerHandle, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		//Error_Handler();
	}
	if (HAL_TIM_OC_ConfigChannel(&timerHandle, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
	{
		//Error_Handler();
	}
	sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
	sConfigOC.Pulse = 1;
	if (HAL_TIM_OC_ConfigChannel(&timerHandle, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
	{
		//Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.BreakFilter = 0;
	sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
	sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
	sBreakDeadTimeConfig.Break2Filter = 0;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&timerHandle, &sBreakDeadTimeConfig) != HAL_OK)
	{
		//Error_Handler();
	}

	PwmIoInit();
	HAL_TIM_PWM_Start(&timerHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&timerHandle, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&timerHandle, TIM_CHANNEL_3);
	HAL_TIMEx_PWMN_Start(&timerHandle, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&timerHandle, TIM_CHANNEL_2);
	HAL_TIMEx_PWMN_Start(&timerHandle, TIM_CHANNEL_3);
}

void HardwareLayer::MotorPwm::PwmIoInit()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    /**TIM1 GPIO Configuration
    PE10     ------> TIM1_CH2N
    PE11     ------> TIM1_CH2
    PE8     ------> TIM1_CH1N
    PE12     ------> TIM1_CH3N
    PE9     ------> TIM1_CH1
    PE13     ------> TIM1_CH3
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_8|GPIO_PIN_12
                          |GPIO_PIN_9|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void MotorPwm::SetPwmChannel1Duty(uint32_t duty)
{
	__HAL_TIM_SET_COMPARE(&timerHandle, TIM_CHANNEL_1, duty);
}

void MotorPwm::SetPwmChannel2Duty(uint32_t duty)
{
	__HAL_TIM_SET_COMPARE(&timerHandle, TIM_CHANNEL_2, duty);
}

void MotorPwm::SetPwmChannel3Duty(uint32_t duty)
{
	__HAL_TIM_SET_COMPARE(&timerHandle, TIM_CHANNEL_3, duty);
}

void MotorPwm::SetPwmChannel4Duty(uint32_t duty)
{
	__HAL_TIM_SET_COMPARE(&timerHandle, TIM_CHANNEL_4, duty);
}
