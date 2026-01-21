#ifndef FLASH_MEMORY_CONTROLLER_HPP
#define FLASH_MEMORY_CONTROLLER_HPP

#include "stm32h7xx_hal.h"
#include "ErrorHandler.hpp"

namespace Common
{}

namespace HardwareLayer
{
	class FlashStorage
	{
	public:
		FlashStorage();
		Common::ErrorType EraseUserSector();
		Common::ErrorType ProgramWord(uint32_t addressOffset, uint32_t* data);
		Common::ErrorType ReadFourBytes(uint32_t addressOffset, uint32_t* data);
		Common::ErrorType ReadNBytes(uint32_t addressOffset, uint32_t* data, uint32_t size, uint8_t wordSize);
		Common::ErrorType ProgramNWords(uint32_t addressOffset, uint32_t* data, uint32_t size, uint8_t wordSize);

	private:
		uint32_t GetSector(uint32_t Address);

		uint32_t FirstSector = 0, NbOfSectors = 0;
		uint32_t address = 0, SECTORError = 0;
		__IO uint32_t MemoryProgramStatus = 0;
		__IO uint64_t data64 = 0;
		uint32_t Index = 0;
		FLASH_EraseInitTypeDef EraseInitStruct;
	};
}
#endif
