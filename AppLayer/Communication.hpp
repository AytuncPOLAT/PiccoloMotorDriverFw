#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <ICallback.hpp>
#include "IUart.hpp"
#include "Crc16.hpp"
#include "SystemData.hpp"
#include "UserInterface.hpp"
#include "FdcanDriver.hpp"
#include "cmsis_os2.h"

namespace AppLayer
{
	enum class INTERFACE : uint8_t
	{
		NONE = 0,
		USB_CDC,
		RS485
		, CAN
	};

	struct __attribute__((packed)) Payload
	{
		uint8_t payload[8];
	};

	struct __attribute__((packed)) DataFrameCanBridge
	{
		Common::CMD_TYPE cmd;
		uint8_t address;
		Payload payload;
		uint16_t checksum;
	};

	struct __attribute__((packed)) RealTimeCommand
	{
		Common::CMD_TYPE cmd;
		uint8_t padding[3];
		uint8_t data[4];
	};

	class Communication
	: public Common::ICallback
	{
	public:

		Communication(Common::IUart& uartRef, Common::IUart& rs485Ref, HardwareLayer::FdCanDriver &canBusRef, Common::SystemData &systemDataRef, UserInterface& userInterfaceRef);

		void OnUsbCdcReceive(uint8_t *Buf, uint32_t Len);
		void OnRs485Receive(uint8_t *Buf, uint32_t Len);

        static void TaskThread(void *argument);

	    void TransmitDataFrame(Common::SerialFrame txFrame);

	    void Init();
		void Respond();
		void SendPingResponse();

		void RegisterCallback(GenericCallback* callBack) override;
		
		QueueHandle_t remoteCommand = NULL;


		Common::SerialFrame serialFrameRx;
		Common::SerialFrame serialFrameTx;	

		Common::CANBusFrame canFrame;

		RealTimeCommand realtimeCommand;

	private:
		Common::IUart &usbCdc;
		Common::IUart &rs485;

		HardwareLayer::FdCanDriver &canBus;

		struct CanCallbackAdapter : public Common::ICallback::GenericCallback
		{
			Communication &parent;
			CanCallbackAdapter(Communication &p) : parent(p) {}
			void OnReceive(uint8_t* Buf, uint32_t Len, void* instance) override { parent.OnCanReceive(Buf, Len); }
			void OnCallback(uint8_t arg) override { (void)arg; }
		} canCallbackAdapter;


		bool isDataReceived;
		INTERFACE interface = INTERFACE::NONE;

		Common::SystemData& systemData;
		Common::ICallback::GenericCallback* callbackHandle;
		UserInterface& userInterface;

		void OnCanReceive(uint8_t* Buf, uint32_t Len);

		Common::UartReceiveAdapter<Communication> usbCdcCallback;
		Common::UartReceiveAdapter<Communication> rs485Callback;

		void ProcessFrame(uint8_t *Buf, uint32_t Len);
	};
}
#endif // COMMUNICATION_HPP
