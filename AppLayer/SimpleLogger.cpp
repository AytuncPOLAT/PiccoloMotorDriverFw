#include "SimpleLogger.hpp"

using namespace AppLayer;

SimpleLogger::SimpleLogger(Common::IUart &uartRef)
: uart(uartRef)
{
	uart.RegisterOnReceiveCallback(this);
}

void SimpleLogger::OnReceiveCallback(uint8_t *Buf, uint32_t Len)
{
	rxByte = Buf[0];
	size = Len;
}

void SimpleLogger::Print(uint8_t *data, uint32_t size)
{
	uart.Transmit(data, size);
}

uint8_t SimpleLogger::ReadByte()
{
	return rxByte;
}
