#ifndef RS485_HPP
#define RS485_HPP

#include "IUart.hpp"
#include "stm32h7xx_hal.h"

namespace HardwareLayer
{
	class Rs485
	: public Common::IUart
	{
	public:
		Rs485();
		void Init();
		uint8_t Transmit(uint8_t *data, uint32_t size) override;
		uint8_t Receive(uint8_t *data, uint32_t size) override;
		void RegisterOnReceiveCallback(Callback* callBack) override;
		void* GetInstance() override;

		IUart::Callback* callbackHandle;
		uint8_t rxBuffer[64];
		UART_HandleTypeDef huart4;
		DMA_HandleTypeDef hdma_uart4_rx;
		DMA_HandleTypeDef hdma_uart4_tx;

		bool txDoneFlag = true;

	private:
	};
}

#endif
