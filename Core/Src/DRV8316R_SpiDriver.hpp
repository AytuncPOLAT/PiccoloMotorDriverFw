#ifndef DRV8316R_SPI_DRIVER
#define DRV8316R_SPI_DRIVER

#include "cstdint"

extern "C"
{
#include "main.h"
#include "cmsis_os.h"

}

struct __attribute__((packed)) InputDataFrame
{
	uint16_t data : 8;
	uint16_t parity : 1;
	uint16_t address : 6;
	uint16_t wR : 1;
};

struct OutputDataFrame
{
	uint16_t data : 8;
	uint16_t status : 8;
};

class Drv8316rSpiDriver
{
public:
	Drv8316rSpiDriver(SPI_HandleTypeDef &spiInstance);
	std::uint8_t
	SendCommand(std::uint8_t address, std::uint8_t data);

	void drv8316_write_reg(uint8_t reg, uint8_t data);

	void SetCurr1();
	void SetCurr2();

private:
	SPI_HandleTypeDef &hspi2;

	InputDataFrame inputDataFrame;
	OutputDataFrame outputDataFrame;
};

#endif //DRV8316R_SPI_DRIVER
