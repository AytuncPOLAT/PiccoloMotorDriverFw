#define ADC_DEBUG

#include "main.h"
#include "BoardConfig/BoardConfig.h"
#include "cmsis_os.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include "Hardware.hpp"
#include "App.hpp"
#include "DebugPrint.hpp"
#include "GateDriver.hpp"
#include "UserInterface.hpp"
#include "SinusPwm.hpp"
#include "PidControl.hpp"

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart5;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes =
	{
		.name = "defaultTask",
		.stack_size = 128 * 4,
		.priority =
			(osPriority_t)osPriorityNormal,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART5_Init(void);

static void MX_TIM1_Init(void);
static void MX_UART4_Init(void);
void StartDefaultTask(void *argument);

int main(void)
{
//	MPU_Config();
	SCB_EnableICache();
	//SCB_EnableDCache();

	HAL_Init();

	MX_GPIO_Init();
	MX_UART5_Init();

	static Hardware hardware;
	static App app(hardware);

	hardware.usbCom.Init();
	hardware.adc.Init();
	hardware.externalQuadEncoder.Init();
	hardware.as5047.Init();
	hardware.rs485.Init();
	hardware.gateDriver.Init();
	hardware.fdcan.Init();

	osKernelInitialize();

	Common::packetQueue = xQueueCreate(5, sizeof(Common::CommPacket));
	
	defaultTaskHandle = osThreadNew(StartDefaultTask, (void*)&app,
									&defaultTask_attributes);

	app.userInterface.Init();
	app.motorControl.Init();
	app.systemDataController.Init();
	app.analogProcessor.Init();
	app.telemetry.Init();
	app.communication.Init();

	hardware.fdcan.SetSingleFilter(app.systemData.configurationData.deviceAddress);

	osKernelStart();
	while (1)
	{
	}
}

void StartDefaultTask(void *argument)
{
	App* app = (App*)argument;

	app->userInterface.SetUiState(UiState::Warning);

	app->hwPtr.adc.Start();

	app->powerSupply.Init();

	for (;;)
	{
		app->powerSupply.Update();
		osDelay(10);
	}
}

static void MX_UART5_Init(void)
{
	huart5.Instance = UART5;
	huart5.Init.BaudRate = 115200;
	huart5.Init.WordLength = UART_WORDLENGTH_8B;
	huart5.Init.StopBits = UART_STOPBITS_1;
	huart5.Init.Parity = UART_PARITY_NONE;
	huart5.Init.Mode = UART_MODE_TX_RX;
	huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart5.Init.OverSampling = UART_OVERSAMPLING_16;
	huart5.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart5.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart5.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart5) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(&huart5, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(&huart5, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(&huart5) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct =
		{0};

	/* GPIO Ports Clock Enable */
	BOARD_ENABLE_GPIO_PORT_CLOCKS();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(BOARD_STATUS_PING_PORT, BOARD_STATUS_PING_PIN, GPIO_PIN_RESET);

	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = BOARD_STATUS_PING_PIN;
	HAL_GPIO_Init(BOARD_STATUS_PING_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = BOARD_STATUS_ERROR_PIN;
	HAL_GPIO_Init(BOARD_STATUS_ERROR_PORT, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = BOARD_STATUS_HEARTBEAT_PIN;
	HAL_GPIO_Init(BOARD_STATUS_HEARTBEAT_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = BOARD_GATE_DRIVER_ENABLE_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOARD_GATE_DRIVER_ENABLE_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(BOARD_GATE_DRIVER_ENABLE_PORT, BOARD_GATE_DRIVER_ENABLE_PIN, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = BOARD_GPIO_PB2_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOARD_GPIO_PB2_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(BOARD_GPIO_PB2_PORT, BOARD_GPIO_PB2_PIN, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = BOARD_GATE_SPI_NSS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOARD_GATE_SPI_NSS_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(BOARD_GATE_SPI_NSS_PORT, BOARD_GATE_SPI_NSS_PIN, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = BOARD_GATE_DRIVER_FAULT_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOARD_GATE_DRIVER_FAULT_PORT, &GPIO_InitStruct);

	/*Configure GPIO pin : PA8 */
	GPIO_InitStruct.Pin = BOARD_MCO_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = BOARD_MCO_AF;
	HAL_GPIO_Init(BOARD_MCO_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = BOARD_MOTOR_DEBUG_PIN;
	HAL_GPIO_Init(BOARD_MOTOR_DEBUG_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(BOARD_MOTOR_DEBUG_PORT, BOARD_MOTOR_DEBUG_PIN, GPIO_PIN_SET);
	GPIO_InitStruct.Pin = BOARD_ADC_DEBUG_PIN;
	HAL_GPIO_Init(BOARD_ADC_DEBUG_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = BOARD_GPIO_PC1_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOARD_GPIO_PC1_PORT, &GPIO_InitStruct);
	HAL_GPIO_WritePin(BOARD_GPIO_PC1_PORT, BOARD_GPIO_PC1_PIN, GPIO_PIN_SET);
}

/* MPU Configuration */

void MPU_Config(void)
{
	MPU_Region_InitTypeDef MPU_InitStruct =
		{0};

	/* Disables the MPU */
	HAL_MPU_Disable();

	/** Initializes and configures the Region and the memory to be protected
	 */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.BaseAddress = 0x0;
	MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
	MPU_InitStruct.SubRegionDisable = 0x87;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	/* Enables the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM23)
	{
		HAL_IncTick();
	}
}

void Error_Handler(void)
{
	__disable_irq();
	while (1)
	{
	}
}
#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif
