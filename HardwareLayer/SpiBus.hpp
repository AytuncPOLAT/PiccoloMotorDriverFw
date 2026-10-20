#ifndef SPI_BUS_HPP
#define SPI_BUS_HPP

#include <cstdint>

extern "C"
{
#include "main.h"
}

namespace HardwareLayer
{
	// SPI2 bus shared by the gate driver ICs (DRV8316R / DRV8353S).
	// 16-bit frames, CPOL = 0, CPHA = 1 (data latched on falling edge), MSB first, hardware NSS.
	class SpiBus
	{
	public:
		SpiBus() = default;
		void Init();

		bool Transfer16(std::uint16_t txWord, std::uint16_t& rxWord);

		SPI_HandleTypeDef& GetHandle() { return hspi; }

	private:
		static constexpr std::uint32_t TIMEOUT_MS = 10;

		SPI_HandleTypeDef hspi {};
		bool isInitialized = false;
	};
}

#endif // SPI_BUS_HPP
