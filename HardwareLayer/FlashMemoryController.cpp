#include "FlashMemoryController.hpp"

using namespace HardwareLayer;

namespace
{
	constexpr uint32_t FLASH_USER_START_ADDR = (FLASH_BASE + (FLASH_SECTOR_SIZE * 7));
	constexpr uint32_t FLASH_USER_END_ADDR = (FLASH_BASE + (FLASH_SECTOR_SIZE * 8) - 1);
	constexpr uint8_t WORD_SIZE = 8u;
}

FlashStorage::FlashStorage()
{}

Common::ErrorType FlashStorage::EraseSector()
{
	SCB_DisableICache();

	/* Unlock the Flash to enable the flash control register access *************/
	HAL_FLASH_Unlock();

	/* Erase the user Flash area
	(area defined by FLASH_USER_START_ADDR and FLASH_USER_END_ADDR) ***********/

	/* Get the 1st sector to erase */
	FirstSector = GetSector(FLASH_USER_START_ADDR);
	/* Get the number of sector to erase from 1st sector*/
	NbOfSectors = GetSector(FLASH_USER_END_ADDR) - FirstSector + 1;

	/* Fill EraseInit structure*/
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Banks         = FLASH_BANK_1;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_4;
	EraseInitStruct.Sector        = FirstSector;
	EraseInitStruct.NbSectors     = NbOfSectors;
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
	{
		return Common::ErrorType::FLASHMEM;
	}

	HAL_FLASH_Lock();

	return Common::ErrorType::OK;
}

Common::ErrorType FlashStorage::ReadFourBytes(uint32_t addressOffset, uint32_t* data)
{
	address = FLASH_USER_START_ADDR + addressOffset;

	*data = *(uint32_t*)address;
	return Common::ErrorType::OK;
}

Common::ErrorType FlashStorage::ProgramWord(uint32_t addressOffset, uint32_t* data)
{
	HAL_FLASH_Unlock();

	address = FLASH_USER_START_ADDR + addressOffset;

	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, ((uint32_t)data)) != HAL_OK)
	{
		return Common::ErrorType::FLASHMEM;
	}

	HAL_FLASH_Lock();

	return Common::ErrorType::OK;
}

Common::ErrorType FlashStorage::ProgramNWords(uint32_t addressOffset, uint32_t* data, uint32_t size)
{
	uint8_t padding = size % WORD_SIZE;

	HAL_FLASH_Unlock();

	address = FLASH_USER_START_ADDR + addressOffset;

	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address, ((uint32_t)data)) == HAL_OK)
	{
		address += 4;
	}
	else
	{
		return Common::ErrorType::FLASHMEM;
	}

	HAL_FLASH_Lock();

	return Common::ErrorType::OK;
}

uint32_t FlashStorage::GetSector(uint32_t Address)
{
  return (address - FLASH_BASE) / FLASH_SECTOR_SIZE;
}
