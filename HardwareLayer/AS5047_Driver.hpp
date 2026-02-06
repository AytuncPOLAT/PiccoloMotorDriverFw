#ifndef AS5047_DRIVER_HPP
#define AS5047_DRIVER_HPP

#include "stm32h7xx_hal.h"
#include "IEncoder.hpp"

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

		void RegisterOnIndexPulseCallback(Callback* callback) override;

	private:
		int16_t offset;
		int position;
		int16_t oldPosition;

		SPI_HandleTypeDef spiHandle;
		uint16_t SPI_Read(uint16_t address);
	};
}

#endif //AS5047_DRIVER_HPP
