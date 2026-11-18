/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         stm32h7xx_hal_msp.c
  * @brief        This file provides code for the MSP Initialization
  *               and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "BoardConfig/BoardConfig.h"

extern DMA_HandleTypeDef hdma_adc1;

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                    /**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{

  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* System interrupt init*/
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}

/**
  * @brief SPI MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(hspi->Instance==SPI2)
  {
    /* USER CODE BEGIN SPI2_MspInit 0 */

    /* USER CODE END SPI2_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SPI2;
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_CLKP;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* Peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();

    BOARD_ENABLE_GPIO_PORT_CLOCKS();
    /**SPI2 GPIO Configuration
    PB9     ------> SPI2_NSS
    PD3     ------> SPI2_SCK
    PC2_C     ------> SPI2_MISO
    PC3_C     ------> SPI2_MOSI
    */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = BOARD_GATE_SPI_NSS_AF;
    GPIO_InitStruct.Pin = BOARD_GATE_SPI_NSS_PIN;
    HAL_GPIO_Init(BOARD_GATE_SPI_NSS_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BOARD_GATE_SPI_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = BOARD_GATE_SPI_SCK_AF;
    HAL_GPIO_Init(BOARD_GATE_SPI_SCK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BOARD_GATE_SPI_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = BOARD_GATE_SPI_MISO_AF;
    HAL_GPIO_Init(BOARD_GATE_SPI_MISO_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BOARD_GATE_SPI_MOSI_PIN;
    GPIO_InitStruct.Alternate = BOARD_GATE_SPI_MOSI_AF;
    HAL_GPIO_Init(BOARD_GATE_SPI_MOSI_PORT, &GPIO_InitStruct);

    /* USER CODE BEGIN SPI2_MspInit 1 */

    /* USER CODE END SPI2_MspInit 1 */

  }

}


/**
  * @brief SPI MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param hspi: SPI handle pointer
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if(hspi->Instance==SPI2)
  {
    /* USER CODE BEGIN SPI2_MspDeInit 0 */

    /* USER CODE END SPI2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SPI2_CLK_DISABLE();

    /**SPI2 GPIO Configuration
    PB9     ------> SPI2_NSS
    PD3     ------> SPI2_SCK
    PC2_C     ------> SPI2_MISO
    PC3_C     ------> SPI2_MOSI
    */
    //HAL_GPIO_DeInit(BOARD_GATE_SPI_NSS_PORT, BOARD_GATE_SPI_NSS_PIN);

    HAL_GPIO_DeInit(BOARD_GATE_SPI_SCK_PORT, BOARD_GATE_SPI_SCK_PIN);

    HAL_GPIO_DeInit(BOARD_GATE_SPI_MISO_PORT, BOARD_GATE_SPI_MISO_PIN);
    HAL_GPIO_DeInit(BOARD_GATE_SPI_MOSI_PORT, BOARD_GATE_SPI_MOSI_PIN);

    /* USER CODE BEGIN SPI2_MspDeInit 1 */

    /* USER CODE END SPI2_MspDeInit 1 */
  }

}

/**
  * @brief TIM_PWM MSP Initialization
  * This function configures the hardware resources used in this example
  * @param htim_pwm: TIM_PWM handle pointer
  * @retval None
  */
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
  if(htim_pwm->Instance==TIM1)
  {
    /* USER CODE BEGIN TIM1_MspInit 0 */

    /* USER CODE END TIM1_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM1_CLK_ENABLE();
    /* USER CODE BEGIN TIM1_MspInit 1 */

    /* USER CODE END TIM1_MspInit 1 */

  }

}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(htim->Instance==TIM1)
  {
    /* USER CODE BEGIN TIM1_MspPostInit 0 */

    /* USER CODE END TIM1_MspPostInit 0 */

    BOARD_ENABLE_GPIO_PORT_CLOCKS();
    /**TIM1 GPIO Configuration
    PE10     ------> TIM1_CH2N
    PE11     ------> TIM1_CH2
    PE8     ------> TIM1_CH1N
    PE12     ------> TIM1_CH3N
    PE9     ------> TIM1_CH1
    PE13     ------> TIM1_CH3
    */
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = BOARD_MOTOR_PWM_CH1N_PIN;
    GPIO_InitStruct.Alternate = BOARD_MOTOR_PWM_CH1N_AF;
    HAL_GPIO_Init(BOARD_MOTOR_PWM_CH1N_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BOARD_MOTOR_PWM_CH1_PIN;
    GPIO_InitStruct.Alternate = BOARD_MOTOR_PWM_CH1_AF;
    HAL_GPIO_Init(BOARD_MOTOR_PWM_CH1_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BOARD_MOTOR_PWM_CH2N_PIN;
    GPIO_InitStruct.Alternate = BOARD_MOTOR_PWM_CH2N_AF;
    HAL_GPIO_Init(BOARD_MOTOR_PWM_CH2N_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BOARD_MOTOR_PWM_CH2_PIN;
    GPIO_InitStruct.Alternate = BOARD_MOTOR_PWM_CH2_AF;
    HAL_GPIO_Init(BOARD_MOTOR_PWM_CH2_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BOARD_MOTOR_PWM_CH3N_PIN;
    GPIO_InitStruct.Alternate = BOARD_MOTOR_PWM_CH3N_AF;
    HAL_GPIO_Init(BOARD_MOTOR_PWM_CH3N_PORT, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = BOARD_MOTOR_PWM_CH3_PIN;
    GPIO_InitStruct.Alternate = BOARD_MOTOR_PWM_CH3_AF;
    HAL_GPIO_Init(BOARD_MOTOR_PWM_CH3_PORT, &GPIO_InitStruct);

    /* USER CODE BEGIN TIM1_MspPostInit 1 */

    /* USER CODE END TIM1_MspPostInit 1 */
  }

}
/**
  * @brief TIM_PWM MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param htim_pwm: TIM_PWM handle pointer
  * @retval None
  */
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* htim_pwm)
{
  if(htim_pwm->Instance==TIM1)
  {
    /* USER CODE BEGIN TIM1_MspDeInit 0 */

    /* USER CODE END TIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM1_CLK_DISABLE();
    /* USER CODE BEGIN TIM1_MspDeInit 1 */

    /* USER CODE END TIM1_MspDeInit 1 */
  }

}

/**
  * @brief UART MSP Initialization
  * This function configures the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */

/**
  * @brief UART MSP De-Initialization
  * This function freeze the hardware resources used in this example
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if(huart->Instance==UART4)
  {
    /* USER CODE BEGIN UART4_MspDeInit 0 */

    /* USER CODE END UART4_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART4_CLK_DISABLE();

    /**UART4 GPIO Configuration
    PA15(JTDI)     ------> UART4_DE
    PC11     ------> UART4_RX
    PC10     ------> UART4_TX
    */
    HAL_GPIO_DeInit(BOARD_RS485_DE_PORT, BOARD_RS485_DE_PIN);

    HAL_GPIO_DeInit(BOARD_RS485_RX_PORT, BOARD_RS485_RX_PIN);
    HAL_GPIO_DeInit(BOARD_RS485_TX_PORT, BOARD_RS485_TX_PIN);

    /* USER CODE BEGIN UART4_MspDeInit 1 */

    /* USER CODE END UART4_MspDeInit 1 */
  }
  else if(huart->Instance==UART5)
  {
    /* USER CODE BEGIN UART5_MspDeInit 0 */

    /* USER CODE END UART5_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART5_CLK_DISABLE();

    /**UART5 GPIO Configuration
    PD2     ------> UART5_RX
    PC12     ------> UART5_TX
    */
    HAL_GPIO_DeInit(BOARD_UART5_RX_PORT, BOARD_UART5_RX_PIN);

    HAL_GPIO_DeInit(BOARD_UART5_TX_PORT, BOARD_UART5_TX_PIN);

    /* USER CODE BEGIN UART5_MspDeInit 1 */

    /* USER CODE END UART5_MspDeInit 1 */
  }

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
