#ifndef ADC_DRIVER_HPP
#define ADC_DRIVER_HPP

#include "stm32h7xx_hal.h"
#include "SystemData.hpp"

namespace HardwareLayer
{
	class AdcDriver
	{
	public:
		AdcDriver();
		void Init();
		void Start();
		uint16_t ReadChannel(uint8_t channel);
		uint16_t ReadChannel(Common::ADC_CHANNELS channel);

	private:
		ADC_HandleTypeDef hadc1;
		DMA_HandleTypeDef hdma_adc1;
	};

}

#endif //ADC_DRIVER_HPP
