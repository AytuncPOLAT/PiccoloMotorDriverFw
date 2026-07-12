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
	Common::RemoteCommand remoteCommand;

	memcpy(&canFrame.registerAddress, Buf, Len);

	remoteCommand.registerAddress = canFrame.registerAddress;
	remoteCommand.command = (uint8_t)canFrame.command;
	memcpy(remoteCommand.data, canFrame.data, 4);

	if(canFrame.sourceID > 1) // Redirect to USBCDC
	{
		serialFrameTx.canFrame.command = Common::CMD_TYPE::PING_RESPONSE;
		serialFrameTx.canFrame.messageID = canFrame.sourceID;
		serialFrameTx.canFrame.registerAddress = 0;
		serialFrameTx.canFrame.data[0] = 0;
		serialFrameTx.canFrame.data[1] = 0;
		serialFrameTx.canFrame.data[2] = 0;
		serialFrameTx.canFrame.data[3] = 0;
		TransmitDataFrame(serialFrameTx);
	}

	else
	{
		if(remoteCommand.command == (uint8_t)Common::CMD_TYPE::PING)
		{
			SendPingResponse();
			userInterface.PingActivity();
		}
		else
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			xQueueSendToBackFromISR(Common::remoteCommandQueue, &remoteCommand, &xHigherPriorityTaskWoken);
			userInterface.CommActivity();
		}
	}
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
			if(serialFrameRx.canFrame.messageID == systemData.configurationData.deviceAddress) // Address match, consume the frame
			{
				Common::RemoteCommand remoteCommand;

				if(serialFrameRx.canFrame.command == Common::CMD_TYPE::PING)
				{
					SendPingResponse();
					userInterface.PingActivity();
				}
				else
				{
					remoteCommand.registerAddress = serialFrameRx.canFrame.registerAddress;
					remoteCommand.command = (uint8_t)serialFrameRx.canFrame.command;
					memcpy(remoteCommand.data, serialFrameRx.canFrame.data, 4);
					xQueueSendToBackFromISR(Common::remoteCommandQueue, &remoteCommand, &xHigherPriorityTaskWoken);

					userInterface.CommActivity();
				}
			}
			else // Redirect
			{
				canBus.AddMessageToTxQueue(serialFrameRx.canFrame.messageID, (uint8_t*)&serialFrameRx.canFrame.data);
				userInterface.CommActivity();
			}
		}
	}
}

void Communication::TransmitDataFrame(Common::SerialFrame txFrame)
{
    Common::Crc16 crc;

    txFrame.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&txFrame), sizeof (txFrame) - 2);

	usbCdc.Transmit((uint8_t*)&txFrame, sizeof(txFrame));
}

void Communication::SendPingResponse()
{
	if(interface == INTERFACE::CAN)
	{
		Common::CANBusFrame canFrame;
		canFrame.sourceID = systemData.configurationData.deviceAddress;
		canFrame.command = Common::CMD_TYPE::PING_RESPONSE;
		canFrame.registerAddress = 0;

		canBus.AddMessageToTxQueue(canFrame.sourceID, (uint8_t*)&canFrame);
		return;
	}

	serialFrameTx.canFrame.command = Common::CMD_TYPE::PING_RESPONSE;
	serialFrameTx.canFrame.messageID = systemData.configurationData.deviceAddress;
	serialFrameTx.canFrame.registerAddress = 0;
	serialFrameTx.canFrame.data[0] = 0;
	serialFrameTx.canFrame.data[1] = 0;
	serialFrameTx.canFrame.data[2] = 0;
	serialFrameTx.canFrame.data[3] = 0;
	TransmitDataFrame(serialFrameTx);
}

void Communication::Respond()
{
	TransmitDataFrame(serialFrameTx);
}
