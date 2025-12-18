#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include "IUart.hpp"
#include "Crc16.hpp"

namespace AppLayer
{
	struct __attribute__((packed)) DataFrame
	{
		uint8_t cmd;
		uint8_t address;
		uint32_t data0;
		uint32_t data1;
		uint32_t data2;
		uint32_t data3;
		uint16_t checksum;
	};

	class Communication
	: public Common::IUart::Callback
	{
	public:
		Communication(Common::IUart& uartRef);
		void OnReceiveCallback(uint8_t *Buf, uint32_t Len) override;
		void Print(uint8_t *data, uint32_t size);
		uint8_t ReadByte();
		void Plot(uint32_t);

	private:
		Common::IUart &uart;
		uint8_t rxByte;
		uint32_t size;
		DataFrame dataFrame;
		DataFrame txDataFrame;
	};
}
#endif // COMMUNICATION_HPP
