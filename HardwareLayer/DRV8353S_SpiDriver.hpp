#ifndef DRV8353S_SPI_DRIVER_HPP
#define DRV8353S_SPI_DRIVER_HPP

#include <cstdint>
#include "SpiBus.hpp"

namespace HardwareLayer
{
	// TI DRV8353S / DRV8353RS smart gate driver (datasheet SLVSDY6A).
	// SDI frame: B15 = R/W (1 = read), B14..B11 = address, B10..B0 = data.
	class Drv8353sSpiDriver
	{
	public:
		enum class Register : std::uint8_t
		{
			FAULT_STATUS_1 = 0x00,
			VGS_STATUS_2   = 0x01,
			DRIVER_CONTROL = 0x02,
			GATE_DRIVE_HS  = 0x03,
			GATE_DRIVE_LS  = 0x04,
			OCP_CONTROL    = 0x05,
			CSA_CONTROL    = 0x06,
			DRIVER_CONFIG  = 0x07,
		};

		// CSA_GAIN, register 0x06 bits 7..6
		enum class CsaGain : std::uint8_t
		{
			GAIN_5_VV  = 0b00,
			GAIN_10_VV = 0b01,
			GAIN_20_VV = 0b10,
			GAIN_40_VV = 0b11,
		};

		// IDRIVEP_HS / IDRIVEP_LS peak source (turn-on) current, registers 0x03/0x04 bits 7..4
		enum class IdriveSource : std::uint8_t
		{
			MA_50   = 0b0001,
			MA_100  = 0b0010,
			MA_150  = 0b0011,
			MA_300  = 0b0100,
			MA_350  = 0b0101,
			MA_400  = 0b0110,
			MA_450  = 0b0111,
			MA_550  = 0b1000,
			MA_600  = 0b1001,
			MA_650  = 0b1010,
			MA_700  = 0b1011,
			MA_850  = 0b1100,
			MA_900  = 0b1101,
			MA_950  = 0b1110,
			MA_1000 = 0b1111,
		};

		// IDRIVEN_HS / IDRIVEN_LS peak sink (turn-off) current, registers 0x03/0x04 bits 3..0
		enum class IdriveSink : std::uint8_t
		{
			MA_100  = 0b0001,
			MA_200  = 0b0010,
			MA_300  = 0b0011,
			MA_600  = 0b0100,
			MA_700  = 0b0101,
			MA_800  = 0b0110,
			MA_900  = 0b0111,
			MA_1100 = 0b1000,
			MA_1200 = 0b1001,
			MA_1300 = 0b1010,
			MA_1400 = 0b1011,
			MA_1700 = 0b1100,
			MA_1800 = 0b1101,
			MA_1900 = 0b1110,
			MA_2000 = 0b1111,
		};

		// TDRIVE peak gate-current drive time, register 0x04 bits 9..8
		enum class Tdrive : std::uint8_t
		{
			NS_500  = 0b00,
			NS_1000 = 0b01,
			NS_2000 = 0b10,
			NS_4000 = 0b11,
		};

		struct GateDriveStrength
		{
			IdriveSource highSideSource;
			IdriveSink   highSideSink;
			IdriveSource lowSideSource;
			IdriveSink   lowSideSink;
			Tdrive       tdrive;
		};

		struct Config
		{
			CsaGain csaGain;
			GateDriveStrength gateDrive;
		};

		// Startup: minimum gate drive current; retain reset CSA gain and drive timeout.
		static constexpr Config DEFAULT_CONFIG =
		{
			CsaGain::GAIN_20_VV,
			{ IdriveSource::MA_50, IdriveSink::MA_100,
			  IdriveSource::MA_50, IdriveSink::MA_100,
			  Tdrive::NS_4000 }
		};

		struct FaultStatus
		{
			std::uint16_t faultStatus1;
			std::uint16_t vgsStatus2;
			bool IsFaulted() const { return (faultStatus1 & (1u << 10)) != 0; }
		};

		explicit Drv8353sSpiDriver(SpiBus& spiBusRef,
								   GPIO_TypeDef* enablePort = GPIOB,
								   std::uint16_t enablePin = GPIO_PIN_11,
								   const Config& initialConfig = DEFAULT_CONFIG);

		// Initializes SPI and drives ENABLE high (device does not respond to SPI while asleep).
		void Init();

		// ENABLE low puts the device in sleep mode; SPI settings must be re-applied with Configure() after wake-up.
		void SetEnabled(bool enabled);
		bool IsEnabled() const { return isEnabled; }

		// Applies the stored configuration and clears latched faults.
		bool Configure();

		bool SetCsaGain(CsaGain gain);
		CsaGain GetCsaGain() const { return config.csaGain; }
		float GetCsaGainVoltsPerVolt() const;

		bool SetGateDriveStrength(const GateDriveStrength& strength);
		bool SetHighSideGateDrive(IdriveSource source, IdriveSink sink);
		bool SetLowSideGateDrive(IdriveSource source, IdriveSink sink, Tdrive tdrive);
		const GateDriveStrength& GetGateDriveStrength() const { return config.gateDrive; }

		bool ClearFaults();
		FaultStatus ReadFaultStatus();

		bool ReadRegister(Register reg, std::uint16_t& value);
		bool WriteRegister(Register reg, std::uint16_t value);

	private:
		static constexpr std::uint16_t DATA_MASK = 0x07FF;
		// tWAKE is 1 ms max (datasheet); keep some margin.
		static constexpr std::uint32_t WAKE_TIME_MS = 2;

		bool ModifyRegister(Register reg, std::uint16_t mask, std::uint16_t value);

		SpiBus& spiBus;
		GPIO_TypeDef* enablePort;
		std::uint16_t enablePin;
		Config config;
		bool isEnabled = false;
	};
}

#endif // DRV8353S_SPI_DRIVER_HPP
