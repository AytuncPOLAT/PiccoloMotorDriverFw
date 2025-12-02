#include "SimpleLogger.hpp"

using namespace AppLayer;

SimpleLogger::SimpleLogger(Common::IUart &uartRef)
: uart(uartRef)
{
	uart.RegisterOnReceiveCallback(this);
}

void SimpleLogger::OnReceiveCallback(uint8_t *Buf, uint32_t Len)
{

}
