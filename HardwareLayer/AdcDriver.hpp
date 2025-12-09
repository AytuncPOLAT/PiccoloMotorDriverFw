#ifndef ADC_DRIVER_HPP
#define ADC_DRIVER_HPP

#include "stm32h7xx_hal.h"

namespace HardwareLayer
{
	class AdcDriver
	{
	public:
		AdcDriver();
		void Init();

	private:
		ADC_HandleTypeDef hadc1;
		DMA_HandleTypeDef hdma_adc1;
	};

}

#endif //ADC_DRIVER_HPP
