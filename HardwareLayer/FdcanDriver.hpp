#ifndef FDCANDRIVER_HPP_
#define FDCANDRIVER_HPP_

#include <cstdint>
extern "C"
{
    #include "stm32h7xx_hal.h"
}

namespace HardwareLayer
{
    class FdCanDriver
    {
    public:
        FdCanDriver();
        void Init();

        void AddMessageToTxQueue();

        FDCAN_HandleTypeDef hfdcan;
        uint8_t RxData[12];
        FDCAN_RxHeaderTypeDef RxHeader;
        bool newData = false;

    private:
        FDCAN_TxHeaderTypeDef TxHeader;
        FDCAN_FilterTypeDef sFilterConfig;
    };
}

#endif /* FDCANDRIVER_HPP_ */
