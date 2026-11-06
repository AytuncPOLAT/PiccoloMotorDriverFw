#include "FlashMemoryController.hpp"
#include "Crc16.hpp"
#include "SystemData.hpp"
#include <cstring>

using namespace HardwareLayer;

namespace
{
	constexpr uint32_t FLASH_USER_START_ADDR = (FLASH_BASE + (FLASH_SECTOR_SIZE * 7));
	constexpr uint32_t FLASH_USER_END_ADDR = (FLASH_BASE + (FLASH_SECTOR_SIZE * 8) - 1);
	constexpr uint32_t FLASH_COGGING_MAP_START_ADDR = (FLASH_BASE + (FLASH_SECTOR_SIZE * 6));
	constexpr uint32_t FLASH_COGGING_MAP_SECTOR = 6U;
	constexpr uint32_t FLASH_COGGING_MAP_MAGIC = 0x434F4747U;
	constexpr uint32_t FLASH_COGGING_MAP_VERSION = 1U;
	constexpr uint32_t FLASH_WORD_BYTES = 32U;
	constexpr uint8_t WORD_SIZE = 8u;

	struct CoggingMapHeader
	{
		uint32_t magic;
		uint32_t version;
		uint32_t sampleCount;
		int32_t torqueMean;
		uint16_t checksum;
		uint16_t reserved16;
		uint32_t reserved32[3];
	};

	alignas(FLASH_WORD_BYTES) uint32_t coggingMapFlashWordBuffer[FLASH_WORD_BYTES / sizeof(uint32_t)] = {};

	static_assert(sizeof(CoggingMapHeader) == FLASH_WORD_BYTES, "Cogging map header must occupy one flashword");
	static_assert((Common::COGGING_MAP_SIZE * sizeof(int32_t)) % FLASH_WORD_BYTES == 0U,
		"Cogging map size must be flashword aligned");

	uint16_t CalculateCoggingMapChecksum(const CoggingMapHeader& header, const int32_t* map)
	{
		Common::Crc16 crc;
		uint8_t* headerData = reinterpret_cast<uint8_t*>(
			const_cast<uint32_t*>(&header.version));
		uint16_t checksum = crc.Calculate(0, headerData,
			static_cast<int>(sizeof(header.version) + sizeof(header.sampleCount) + sizeof(header.torqueMean)));
		uint8_t* mapData = reinterpret_cast<uint8_t*>(const_cast<int32_t*>(map));
		return crc.Calculate(checksum, mapData,
			static_cast<int>(Common::COGGING_MAP_SIZE * sizeof(int32_t)));
	}
}

FlashStorage::FlashStorage()
{}

Common::ErrorType FlashStorage::EraseUserSector()
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

Common::ErrorType FlashStorage::SaveCoggingMap(const int32_t* map, int32_t torqueMean)
{
	CoggingMapHeader header = {};
	header.magic = FLASH_COGGING_MAP_MAGIC;
	header.version = FLASH_COGGING_MAP_VERSION;
	header.sampleCount = Common::COGGING_MAP_SIZE;
	header.torqueMean = torqueMean;
	header.checksum = CalculateCoggingMapChecksum(header, map);

	SCB_DisableICache();
	HAL_FLASH_Unlock();

	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Banks = FLASH_BANK_1;
	EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_4;
	EraseInitStruct.Sector = FLASH_COGGING_MAP_SECTOR;
	EraseInitStruct.NbSectors = 1U;
	if (HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
	{
		HAL_FLASH_Lock();
		return Common::ErrorType::FLASHMEM;
	}

	const uint8_t* mapBytes = reinterpret_cast<const uint8_t*>(map);
	const uint32_t mapSize = Common::COGGING_MAP_SIZE * sizeof(int32_t);
	for (uint32_t offset = 0; offset < mapSize; offset += FLASH_WORD_BYTES)
	{
		std::memcpy(coggingMapFlashWordBuffer, mapBytes + offset, FLASH_WORD_BYTES);
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
			FLASH_COGGING_MAP_START_ADDR + FLASH_WORD_BYTES + offset,
			reinterpret_cast<uint32_t>(coggingMapFlashWordBuffer)) != HAL_OK)
		{
			HAL_FLASH_Lock();
			return Common::ErrorType::FLASHMEM;
		}
	}

	// Write the valid header last so interrupted writes are rejected on startup.
	std::memcpy(coggingMapFlashWordBuffer, &header, sizeof(header));
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, FLASH_COGGING_MAP_START_ADDR,
		reinterpret_cast<uint32_t>(coggingMapFlashWordBuffer)) != HAL_OK)
	{
		HAL_FLASH_Lock();
		return Common::ErrorType::FLASHMEM;
	}

	HAL_FLASH_Lock();
	return Common::ErrorType::OK;
}

bool FlashStorage::LoadCoggingMap(int32_t* map, int32_t& torqueMean) const
{
	const auto* header = reinterpret_cast<const CoggingMapHeader*>(FLASH_COGGING_MAP_START_ADDR);
	if (header->magic != FLASH_COGGING_MAP_MAGIC ||
		header->version != FLASH_COGGING_MAP_VERSION ||
		header->sampleCount != Common::COGGING_MAP_SIZE)
	{
		return false;
	}

	const auto* storedMap = reinterpret_cast<const int32_t*>(
		FLASH_COGGING_MAP_START_ADDR + sizeof(CoggingMapHeader));
	if (CalculateCoggingMapChecksum(*header, storedMap) != header->checksum)
	{
		return false;
	}

	std::memcpy(map, storedMap, Common::COGGING_MAP_SIZE * sizeof(int32_t));
	torqueMean = header->torqueMean;
	return true;
}

Common::ErrorType FlashStorage::ReadFourBytes(uint32_t addressOffset, uint32_t* data)
{
	SCB_DisableICache();
	HAL_FLASH_Unlock();

	address = FLASH_USER_START_ADDR + addressOffset;
	*data = *(uint32_t*)address;

	HAL_FLASH_Lock();

	return Common::ErrorType::OK;
}

Common::ErrorType FlashStorage::ReadNBytes(uint32_t addressOffset, uint32_t* data, uint32_t size, uint8_t wordSize)
{
	uint8_t padding = size % wordSize;
	uint8_t numberOfWords = size / wordSize;

	SCB_DisableICache();
	HAL_FLASH_Unlock();

	address = FLASH_USER_START_ADDR + addressOffset;

	for(uint8_t i = 0; i < numberOfWords; i++)
	{
		*(data + i) = *(uint32_t*)(address + i * wordSize);
	}
	HAL_FLASH_Lock();

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

Common::ErrorType FlashStorage::ProgramNWords(uint32_t addressOffset, uint32_t* data, uint32_t size, uint8_t wordSize)
{
	uint8_t padding = size % wordSize;
	uint8_t numberOfWords = size / wordSize;

	HAL_FLASH_Unlock();

	address = FLASH_USER_START_ADDR + addressOffset;

	for(uint8_t i = 0; i < numberOfWords; i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, (address + (i * wordSize)), ((uint32_t)data + i * wordSize)) != HAL_OK)
		{
			return Common::ErrorType::FLASHMEM;
		}
	}

	HAL_FLASH_Lock();

	return Common::ErrorType::OK;
}

uint32_t FlashStorage::GetSector(uint32_t address)
{
  return (address - FLASH_BASE) / FLASH_SECTOR_SIZE;
}
