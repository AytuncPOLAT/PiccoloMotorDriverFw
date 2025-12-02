#ifndef DRV8316R_SPI_DRIVER
#define DRV8316R_SPI_DRIVER

#include "cstdint"

extern "C"
{
#include "main.h"
#include "cmsis_os.h"

}

class Drv8316rSpiDriver
{
public:
	Drv8316rSpiDriver(SPI_HandleTypeDef &spiInstance);
	std::uint8_t
	SendCommand(std::uint8_t address, std::uint8_t data);
private:
	SPI_HandleTypeDef &hspi2;
};

#endif //DRV8316R_SPI_DRIVER
