#ifndef AS5047_DRIVER_HPP
#define AS5047_DRIVER_HPP

#include "stm32h7xx_hal.h"
#include "IEncoder.hpp"
#include "SignalProcessing.hpp"

namespace HardwareLayer
{
	class AS5047
	: public IEncoder
	{
	public:
		AS5047();
		void Init();

		void Reset() override;
		int GetPosition() override;
		float GetRotorAngleInRadians() override;
		void SetRotorEncoderOffset(int16_t newOffset) override;
		int GetSpeed() override;
		int GetMultiTurnPosition() override;
		void RegisterOnIndexPulseCallback(Callback* callback) override;

	    SPI_HandleTypeDef* GetSpiHandle() { return &spiHandle; }
	    void OnTransferComplete();
	    void StartAsyncRead();          // kick off a new transfer

	    int GetPosition_Async();

	    SPI_HandleTypeDef spiHandle;
	    DMA_HandleTypeDef hdma_spi3_rx;
	    DMA_HandleTypeDef hdma_spi3_tx;

	private:
		int16_t offset;
		int position;
		int oldPosMultiTurn;
		int multiTurnRev = 0;
		int16_t oldPosition;

		AppLayer::LowPassFilter speedFilter;



		uint16_t SPI_Read(uint16_t address);
		void DMA_Init();

	    volatile uint16_t dmaTxBuf;
	    volatile uint16_t dmaRxBuf;

	    volatile bool dmaTransferDone = false;
	};
}

#endif //AS5047_DRIVER_HPP
