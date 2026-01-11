#include "Communication.hpp"
#include <cstring>

using namespace AppLayer;

namespace
{

}

Communication::Communication(Common::IUart& uartRef, Common::SystemData &systemDataRef)
: uart(uartRef)
, systemData(systemDataRef)
{
	uart.RegisterOnReceiveCallback(this);
}

void Communication::RegisterCallback(Common::ICallback::GenericCallback* callback)
{
	callbackHandle = callback;
}

void Communication::OnReceiveCallback(uint8_t *Buf, uint32_t Len)
{
	Common::Crc16 crc;

	if(Len == sizeof(dataFrame))
	{
		memcpy((void*)&dataFrame, (void*)Buf, sizeof(dataFrame));
		auto localChecksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&dataFrame), sizeof (dataFrame) - 2);
		if(dataFrame.checksum == localChecksum)
		{
			if(dataFrame.address == systemData.configurationData.deviceAddress)
			{
				memcpy((void*)&rxData, (void*)&dataFrame, sizeof(dataFrame));
				isDataReceived = true;

				Filters(Len);
			}
		}
	}
	rxByte = Buf[0];
	size = Len;
}

void Communication::Filters(uint16_t len)
{
	if(rxData.cmd == CMD_TYPE::PING)
	{
		SendPingResponse();
	}
	else
		callbackHandle->OnCallback(len);
}

void Communication::TransmitDataFrame(CMD_TYPE cmd,
                                uint32_t deviceAddress,
                                uint32_t data0,
                                uint32_t data1,
                                uint32_t data2,
                                uint32_t data3)
{
	txData.cmd = cmd;
	txData.address = deviceAddress;
    txData.data0 = data0;
    txData.data1 = data1;
    txData.data2 = data2;
    txData.data3 = data3;

    Common::Crc16 crc;
    txData.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&txData), sizeof (txData) - 2);

    uart.Transmit((uint8_t*)&txData, sizeof(txData));
}

void Communication::SendPingResponse()
{
	TransmitDataFrame(CMD_TYPE::PING_RESPONSE,
			systemData.configurationData.deviceAddress, 0, 0, 0, 0);
}

void Communication::Print(uint8_t *data, uint32_t size)
{
	uart.Transmit(data, size);
}

void Communication::Plot(uint32_t data)
{
	txData.address = 1;
	txData.cmd = CMD_TYPE::WRITE_TO_DEVICE;
	txData.data0 = data;
	uart.Transmit((uint8_t*)&txData, sizeof(txData));
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

void Communication::TransmitTxFrame()
{
	uart.Transmit((uint8_t*)&txData, sizeof(txData));
}
