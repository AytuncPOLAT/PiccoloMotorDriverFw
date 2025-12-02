#include "DRV8316R_SpiDriver.hpp"
#include "DebugPrint.hpp"

Drv8316rSpiDriver::Drv8316rSpiDriver(SPI_HandleTypeDef &spiInstance) :
		hspi2(spiInstance)
{

}

std::uint8_t Drv8316rSpiDriver::SendCommand(std::uint8_t address,
		std::uint8_t data)
{
	uint8_t txData[2] =
	{ 0x80, 0x00 };
	uint8_t rxData[2] =
	{ 0x00, 0x80 };
	//std::uint8_t checksum = data[0]%2;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	osDelay(1);
	HAL_SPI_TransmitReceive(&hspi2, txData, rxData, 2, 1000);
	osDelay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	uint8_t buffer[20];
	uint16_t size = 0;
	memset(buffer, 0, sizeof(buffer));
	size = sprintf((char*) buffer, "rx80 = %d, %d\r\n", rxData[0], rxData[1]);
	//Service::DebugPrint::Instance().Print(buffer, size);

	osDelay(1);

	txData[0] = 0x10;
	txData[1] = 0x11;
	//std::uint8_t checksum = data[0]%2;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	osDelay(1);
	HAL_SPI_TransmitReceive(&hspi2, txData, rxData, 2, 1000);
	osDelay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	buffer[20];
	size = 0;
	memset(buffer, 0, sizeof(buffer));
	size = sprintf((char*) buffer, "rx82 = %d, %d\r\n", rxData[0], rxData[1]);
	//Service::DebugPrint::Instance().Print(buffer, size);

	txData[0] = 0x10;
	txData[1] = 0x11;
	//std::uint8_t checksum = data[0]%2;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	osDelay(1);
	HAL_SPI_TransmitReceive(&hspi2, txData, rxData, 2, 1000);
	osDelay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	buffer[20];
	size = 0;
	memset(buffer, 0, sizeof(buffer));
	size = sprintf((char*) buffer, "rx82 = %d, %d\r\n", rxData[0], rxData[1]);
	//Service::DebugPrint::Instance().Print(buffer, size);

	txData[0] = 0x90;
	//std::uint8_t checksum = data[0]%2;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
	osDelay(1);
	HAL_SPI_TransmitReceive(&hspi2, txData, rxData, 2, 1000);
	osDelay(1);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
	buffer[20];
	size = 0;
	memset(buffer, 0, sizeof(buffer));
	size = sprintf((char*) buffer, "rx82 = %d, %d\r\n", rxData[0], rxData[1]);
	//Service::DebugPrint::Instance().Print(buffer, size);

	return 0;
}
