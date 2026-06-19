#ifndef FDCANDRIVER_HPP_
#define FDCANDRIVER_HPP_

#include "ICallback.hpp"

#include <cstdint>
extern "C"
{
    #include "stm32h7xx_hal.h"
}

namespace HardwareLayer
{
    class FdCanDriver : public Common::ICallback
    {
    public:
        FdCanDriver();
        void Init();

        void AddMessageToTxQueue();

        void RegisterCallback(Common::ICallback::GenericCallback* callBack) override;

        FDCAN_HandleTypeDef hfdcan;
        uint8_t RxData[12];
        FDCAN_RxHeaderTypeDef RxHeader;
        bool newData = false;
        Common::ICallback::GenericCallback* callbackHandle = nullptr;

    private:
        FDCAN_TxHeaderTypeDef TxHeader;
        FDCAN_FilterTypeDef sFilterConfig;
    };
}

#endif /* FDCANDRIVER_HPP_ */
