#include "Communication.hpp"
#include <cstring>

using namespace AppLayer;

namespace
{

}

Communication::Communication(Common::IUart& uartRef)
: uart(uartRef)
{
	uart.RegisterOnReceiveCallback(this);
}

void Communication::OnReceiveCallback(uint8_t *Buf, uint32_t Len)
{
	Common::Crc16 crc;

	if(Len == sizeof(dataFrame))
	{
		memcpy((void*)&dataFrame, (void*)Buf, sizeof(dataFrame));
		auto localChecksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&dataFrame), sizeof (dataFrame) - 2);
		//if(dataFrame.checksum == localChecksum)
		//{
		//	volatile int dummy = 0;
		//}
		if(dataFrame.address == 101)
		{
			isDataReceived = true;
		}
	}
	rxByte = Buf[0];
	size = Len;
}

void Communication::Print(uint8_t *data, uint32_t size)
{
	uart.Transmit(data, size);
}

void Communication::Plot(uint32_t data)
{
	txDataFrame.address = 1;
	txDataFrame.cmd = 1;
	txDataFrame.data0 = data;
	uart.Transmit((uint8_t*)&txDataFrame, sizeof(txDataFrame));
}

uint8_t Communication::ReadByte()
{
	return rxByte;
}

bool Communication::GetRxStatus()
{
	bool returnValue = isDataReceived;
	isDataReceived = false;
	return returnValue;
}

uint32_t Communication::GetPayload()
{
	return dataFrame.data0;
}
