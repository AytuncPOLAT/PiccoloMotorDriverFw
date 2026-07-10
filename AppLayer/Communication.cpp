#include "Communication.hpp"
#include <cstring>
#include "SystemData.hpp"

using namespace AppLayer;

namespace
{

}

Communication::Communication(Common::IUart& uartRef,
							Common::IUart& rs485Ref,
							HardwareLayer::FdCanDriver &canBusRef,
							Common::SystemData &systemDataRef,
							UserInterface& userInterfaceRef)
: usbCdc(uartRef)
, rs485(rs485Ref)
, canBus(canBusRef)
, systemData(systemDataRef)
, userInterface(userInterfaceRef)
, usbCdcCallback(*this, &Communication::OnUsbCdcReceive)
, rs485Callback(*this, &Communication::OnRs485Receive)
, canCallbackAdapter(*this)
{
	usbCdc.RegisterOnReceiveCallback(&usbCdcCallback);
	rs485.RegisterOnReceiveCallback(&rs485Callback);
	canBus.RegisterCallback(&canCallbackAdapter);
}

void Communication::Init()
{
	/*
	xTaskCreate(this->TaskThread, "commTask", 128 * 4, (void*) this,
			24, NULL);
	*/
}

void Communication::TaskThread(void *argument)
{
	Communication *objectHandle = static_cast<Communication*>(argument);

	while(1)
	{
		osDelay(100);
	}
}

void Communication::RegisterCallback(Common::ICallback::GenericCallback* callback)
{
	callbackHandle = callback;
}

void Communication::OnUsbCdcReceive(uint8_t *Buf, uint32_t Len)
{
	interface = INTERFACE::USB_CDC;
	ProcessFrame(Buf, Len);
}

void Communication::OnRs485Receive(uint8_t *Buf, uint32_t Len)
{
	interface = INTERFACE::RS485;
	ProcessFrame(Buf, Len);
}

void Communication::OnCanReceive(uint8_t* Buf, uint32_t Len)
{
	interface = INTERFACE::CAN;

	NotifySystemData((AppLayer::CMD_TYPE)Buf[0], &Buf[4]);
	userInterface.CommActivity();
}

void Communication::ProcessFrame(uint8_t *Buf, uint32_t Len)
{
	Common::Crc16 crc;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(Len == sizeof(Common::SerialFrame)) // Serial packet
	{
		memcpy((void*)&serialFrameRx, (void*)Buf, sizeof(Common::SerialFrame));
		auto localChecksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&serialFrameRx), sizeof (serialFrameRx) - 2);

		if(serialFrameRx.checksum == localChecksum) // Valid frame 
		{
			if(serialFrameRx.canFrame.msgID == systemData.configurationData.deviceAddress) // Address match, consume the frame
			{
				Common::RemoteCommand remoteCommand;

				remoteCommand.subAddress = serialFrameRx.canFrame.subAddress;
				remoteCommand.flags = serialFrameRx.canFrame.flags;
				memcpy(remoteCommand.data, serialFrameRx.canFrame.data, 4);
				xQueueSendToBackFromISR(Common::remoteCommandQueue, &remoteCommand, &xHigherPriorityTaskWoken);

				userInterface.PingActivity();
				userInterface.CommActivity();
			}
			else // Redirect to CAN
			{
				uint8_t canPayload[8];
				memcpy(canPayload, (uint8_t*)&serialFrameRx.canFrame.data, 8);
				canBus.AddMessageToTxQueue(serialFrameRx.canFrame.subAddress, canPayload);
				userInterface.PingActivity();
			}
		}
	}

	
/*
	else if(Len == sizeof(dataFrame)) // Configuration packet
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

	else if(Len == sizeof(canOverSerialData)) // Realtime packet
	{
		memcpy((void*)&canOverSerialData, (void*)Buf, sizeof(canOverSerialData));
		auto localChecksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&canOverSerialData), sizeof (canOverSerialData) - 2);
		if(canOverSerialData.checksum == localChecksum)
		{
			if(canOverSerialData.address == systemData.configurationData.deviceAddress) // Address match
			{
				NotifySystemData(canOverSerialData.cmd, &canOverSerialData.payload.payload[4]);
				userInterface.CommActivity();
			}
			else // Redirect to CAN
			{
				uint8_t canPayload[8];
				memcpy(canPayload, (uint8_t*)&canOverSerialData.payload, 8);
				canBus.AddMessageToTxQueue(canOverSerialData.address, canPayload);
				userInterface.PingActivity();
			}
		}
	}*/
}

void Communication::NotifySystemData(AppLayer::CMD_TYPE cmd, uint8_t* data)
{
	realtimeCommand.cmd = cmd;
	memcpy(realtimeCommand.data, data, 4);
	callbackHandle->OnCallback(111);
}

void Communication::Filters(uint16_t len)
{
	if(rxData.cmd == CMD_TYPE::PING)
	{
		userInterface.PingActivity();
		SendPingResponse();
	}
	else
	{
		callbackHandle->OnCallback(222);
		userInterface.CommActivity();
	}
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

	if(interface == INTERFACE::RS485)
	{
		rs485.Transmit((uint8_t*)&txData, sizeof(txData));
	}
	else if(interface == INTERFACE::USB_CDC)
	{
		usbCdc.Transmit((uint8_t*)&txData, sizeof(txData));
	}
}

void Communication::SendPingResponse()
{
	TransmitDataFrame(CMD_TYPE::PING_RESPONSE,
			systemData.configurationData.deviceAddress, 0, 0, 0, 0);
}

void Communication::Print(uint8_t *data, uint32_t size)
{
	usbCdc.Transmit(data, size);
}

void Communication::TransmitTxFrame()
{
	Common::Crc16 crc;
	txData.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&txData), sizeof (txData) - 2);
	
	if(interface == INTERFACE::RS485)
	{
		rs485.Transmit((uint8_t*)&txData, sizeof(txData));
	}
	else if(interface == INTERFACE::USB_CDC)
	{
		usbCdc.Transmit((uint8_t*)&txData, sizeof(txData));
	}
}
