#include "FdcanDriver.hpp"

using namespace HardwareLayer;

namespace
{
    FdCanDriver* global_fdcan;
}

extern "C"
{
    void FDCAN1_IT0_IRQHandler(void)
    {
        HAL_FDCAN_IRQHandler(&global_fdcan->hfdcan);
    }

    void FDCAN1_IT1_IRQHandler(void)
    {
        HAL_FDCAN_IRQHandler(&global_fdcan->hfdcan);
    }

    void HAL_FDCAN_RxBufferNewMessageCallback(FDCAN_HandleTypeDef *hcan)
    {
    	HAL_FDCAN_GetRxMessage(hcan, FDCAN_RX_BUFFER0, &global_fdcan->RxHeader, global_fdcan->RxData);
    	global_fdcan->newData = true;
    }
}

FdCanDriver::FdCanDriver()
{
    global_fdcan = this;
}

void FdCanDriver::Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInitStruct.FdcanClockSelection = RCC_FDCANCLKSOURCE_PLL;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        //Error_Handler();
    }

    __HAL_RCC_FDCAN_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /**FDCAN1 GPIO Configuration
    PD0     ------> FDCAN1_RX
    PD1     ------> FDCAN1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    hfdcan.Instance = FDCAN1;
    hfdcan.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan.Init.AutoRetransmission = DISABLE;
    hfdcan.Init.TransmitPause = DISABLE;
    hfdcan.Init.ProtocolException = DISABLE;
    hfdcan.Init.NominalPrescaler = 25;
    hfdcan.Init.NominalSyncJumpWidth = 1;
    hfdcan.Init.NominalTimeSeg1 = 2;
    hfdcan.Init.NominalTimeSeg2 = 1;
    hfdcan.Init.DataPrescaler = 1;
    hfdcan.Init.DataSyncJumpWidth = 1;
    hfdcan.Init.DataTimeSeg1 = 1;
    hfdcan.Init.DataTimeSeg2 = 1;
    hfdcan.Init.MessageRAMOffset = 0;
    hfdcan.Init.StdFiltersNbr = 1;
    hfdcan.Init.ExtFiltersNbr = 0;
    hfdcan.Init.RxFifo0ElmtsNbr = 1;
    hfdcan.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
    hfdcan.Init.RxFifo1ElmtsNbr = 0;
    hfdcan.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
    hfdcan.Init.RxBuffersNbr = 1;
    hfdcan.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
    hfdcan.Init.TxEventsNbr = 0;
    hfdcan.Init.TxBuffersNbr = 32;
    hfdcan.Init.TxFifoQueueElmtsNbr = 0;
    hfdcan.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    hfdcan.Init.TxElmtSize = FDCAN_DATA_BYTES_8;

    HAL_FDCAN_Init(&hfdcan);

    /* Configure standard ID reception filter to Rx buffer 0 */
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_DUAL;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXBUFFER;
    sFilterConfig.FilterID1 = 0x111;
    sFilterConfig.FilterID2 = 0x555;
    sFilterConfig.RxBufferIndex = 0;
    HAL_FDCAN_ConfigFilter(&hfdcan, &sFilterConfig);

    /* Configure extended ID reception filter to Rx FIFO 1 */
    //sFilterConfig.IdType = FDCAN_EXTENDED_ID;
    //sFilterConfig.FilterIndex = 0;
    //sFilterConfig.FilterType = FDCAN_FILTER_RANGE_NO_EIDM;
    //sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO1;
    //sFilterConfig.FilterID1 = 0x1111111;
    //sFilterConfig.FilterID2 = 0x2222222;
    //HAL_FDCAN_ConfigFilter(&hfdcan, &sFilterConfig);

    /* Configure Tx buffer message */
    uint8_t TxData0[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    TxHeader.Identifier = 0x11;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    TxHeader.MessageMarker = 0x0;
    HAL_FDCAN_AddMessageToTxBuffer(&hfdcan, &TxHeader, TxData0, FDCAN_TX_BUFFER0);


    HAL_FDCAN_ActivateNotification(&hfdcan, FDCAN_IT_RX_BUFFER_NEW_MESSAGE, FDCAN_IT_TX_COMPLETE);

    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 0, 1);
    HAL_NVIC_SetPriority(FDCAN1_IT1_IRQn, 0, 1);
    HAL_NVIC_SetPriority(FDCAN_CAL_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    HAL_NVIC_EnableIRQ(FDCAN1_IT1_IRQn);
    //HAL_NVIC_EnableIRQ(FDCAN_CAL_IRQn);

    /* Start the FDCAN module */
    HAL_FDCAN_Start(&hfdcan);

    HAL_FDCAN_EnableTxBufferRequest(&hfdcan, FDCAN_TX_BUFFER0);

    HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_BUFFER0, &RxHeader, RxData);
}

void FdCanDriver::AddMessageToTxQueue()
{
    uint8_t TxData1[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    TxHeader.Identifier = 0x01;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_PASSIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    TxHeader.MessageMarker = 0x0;
    HAL_FDCAN_AddMessageToTxBuffer(&hfdcan, &TxHeader, TxData1, FDCAN_TX_BUFFER0);
    HAL_FDCAN_EnableTxBufferRequest(&hfdcan, FDCAN_TX_BUFFER0);
}
