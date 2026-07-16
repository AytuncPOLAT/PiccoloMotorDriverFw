#include "Communication.hpp"
#include <cstring>
#include "SystemData.hpp"

// Layer-2 (Comm bridge) communication logic.
// Handles packets come from Layer-1 (Phy) and redirect to Layer-3 (App) when needed

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
	ProcessFrame(Buf, Len);
}

void Communication::OnRs485Receive(uint8_t *Buf, uint32_t Len)
{
	ProcessFrame(Buf, Len);
}

void Communication::OnCanReceive(uint8_t* Buf, uint32_t Len)
{
	Common::CommPacket packet;

	packet.interface = Common::INTERFACE::CAN;

	memcpy(&packet.frame.canFrame.sourceID, Buf, Len);

	if(packet.frame.canFrame.flags == 0x02) // Redirected packet
	{
		packet.frame.canFrame.messageID = packet.frame.canFrame.sourceID;
		packet.frame.canFrame.registerAddress = packet.frame.canFrame.registerAddress;
		memcpy(&packet.frame.canFrame.data, &packet.frame.canFrame.data[0], 4);
		packet.interface = Common::INTERFACE::USB_CDC; //Change the interface
		TransmitDataFrame(packet);
	}

	else if(packet.frame.canFrame.flags == 0x01) // First CAN request e.g. read req
	{
		if(packet.frame.canFrame.command == Common::CMD_TYPE::PING)
		{
			SendPingResponse(packet);
			userInterface.PingActivity();
		}
		else
		{
			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			xQueueSendToBackFromISR(Common::packetQueue, &packet, &xHigherPriorityTaskWoken);
			userInterface.CommActivity();
		}
	}
}

void Communication::ProcessFrame(uint8_t *Buf, uint32_t Len)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	Common::CommPacket packet;
	packet.interface = Common::INTERFACE::USB_CDC;


	bool isPacketValid = LoadAndValidateSerialFrame(packet, Buf, Len);

	if(isPacketValid)
	{
		if(packet.frame.canFrame.messageID == systemData.configurationData.deviceAddress) // Address match, consume the frame
		{
			if(packet.frame.canFrame.command == Common::CMD_TYPE::PING)
			{
				SendPingResponse(packet);
				userInterface.PingActivity();
			}
			else
			{
				//remoteCommand.registerAddress = serialPacket.canFrame.registerAddress;
				//remoteCommand.command = (uint8_t)serialPacket.canFrame.command;
				//memcpy(remoteCommand.data, serialPacket.canFrame.data, 4);
				xQueueSendToBackFromISR(Common::packetQueue, &packet, &xHigherPriorityTaskWoken);

				userInterface.CommActivity();
			}
		}
		else // Redirect to CANBus
		{
			packet.frame.canFrame.flags = 0x01; //First redirection

			//Stamp own address. Target node is going to use it to find the redirecting node.
			packet.frame.canFrame.sourceID = systemData.configurationData.deviceAddress;

			canBus.AddMessageToTxQueue(packet.frame.canFrame.messageID, (uint8_t*)&packet.frame.canFrame.sourceID);

			userInterface.CommActivity();
		}
	}

}

bool Communication::LoadAndValidateSerialFrame(Common::CommPacket &packet, uint8_t *Buf, uint32_t Len)
{
	Common::Crc16 crc;

	if(Len == sizeof(Common::SerialFrame)) // Serial packet
	{
		memcpy((void*)&packet.frame, (void*)Buf, sizeof(Common::SerialFrame));
		auto localChecksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&packet.frame), sizeof (packet.frame) - 2);

		if(packet.frame.checksum == localChecksum) // Valid frame
		{
			return true;
		}
	}

	return false;
}

void Communication::TransmitDataFrame(Common::CommPacket packet)
{
    Common::Crc16 crc;

    if(packet.interface == Common::INTERFACE::USB_CDC)
    {
    	packet.frame.checksum = crc.Calculate(0, reinterpret_cast<uint8_t*>(&packet.frame), sizeof (packet.frame) - 2);

		usbCdc.Transmit((uint8_t*)&packet.frame, sizeof(packet.frame));
    }
}

void Communication::SendPingResponse(Common::CommPacket packet)
{
	if(packet.interface == Common::INTERFACE::CAN)
	{
		packet.frame.canFrame.flags = 0x02; // Mark return path;
		packet.frame.canFrame.command = Common::CMD_TYPE::PING_RESPONSE;
		packet.frame.canFrame.registerAddress = 0;
		packet.frame.canFrame.messageID = packet.frame.canFrame.sourceID; //Swap the source address with target address (msgID)
		packet.frame.canFrame.sourceID = systemData.configurationData.deviceAddress;

		canBus.AddMessageToTxQueue(packet.frame.canFrame.messageID, (uint8_t*)&packet.frame.canFrame.sourceID);
		return;
	}

	else if(packet.interface == Common::INTERFACE::USB_CDC)
	{
		packet.frame.canFrame.command = Common::CMD_TYPE::PING_RESPONSE;
		packet.frame.canFrame.messageID = systemData.configurationData.deviceAddress;
		packet.frame.canFrame.registerAddress = 0;
		packet.frame.canFrame.data[0] = 0;
		packet.frame.canFrame.data[1] = 0;
		packet.frame.canFrame.data[2] = 0;
		packet.frame.canFrame.data[3] = 0;
		TransmitDataFrame(packet);
	}
}

void Communication::Respond(Common::CommPacket packet)
{
	if(packet.interface == Common::INTERFACE::CAN)
	{
		packet.frame.canFrame.flags = 0x02; // Mark return path;
		packet.frame.canFrame.command = Common::CMD_TYPE::READ_FROM_DEVICE;
		packet.frame.canFrame.messageID = packet.frame.canFrame.sourceID; //Swap the source address with target address (msgID)
		packet.frame.canFrame.sourceID = systemData.configurationData.deviceAddress;
		canBus.AddMessageToTxQueue(packet.frame.canFrame.messageID, (uint8_t*)&packet.frame.canFrame.sourceID);
	}

	else
		TransmitDataFrame(packet);
}
