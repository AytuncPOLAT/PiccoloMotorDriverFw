#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <ICallback.hpp>
#include "IUart.hpp"
#include "Crc16.hpp"
#include "SystemData.hpp"
#include "UserInterface.hpp"
#include "FdcanDriver.hpp"

namespace AppLayer
{
	enum class INTERFACE : uint8_t
	{
		NONE = 0,
		USB_CDC,
		RS485
		, CAN
	};

	enum class CMD_TYPE : uint8_t
	{
	    PING = 0,
	    PING_RESPONSE,
	    READ_FROM_DEVICE,
	    WRITE_TO_DEVICE,
	    WRITE_TO_DEVICE_FLASH,
		MOTION_COMMAND,
		READ_REALTIME,
		DRIVER_ARM,
		DRIVER_DISARM,
		POSITION_HOME,
	    CURR_1,
	    CURR_2
	};

	struct __attribute__((packed)) DataFrame
	{
		CMD_TYPE cmd;
		uint8_t address;
		uint32_t data0;
		uint32_t data1;
		uint32_t data2;
		uint32_t data3;
		uint16_t checksum;
	};

	class Communication
	: public Common::ICallback
	{
	public:
		Communication(Common::IUart& uartRef, Common::IUart& rs485Ref, HardwareLayer::FdCanDriver &canBusRef, Common::SystemData &systemDataRef, UserInterface& userInterfaceRef);
		void OnUsbCdcReceive(uint8_t *Buf, uint32_t Len);
		void OnRs485Receive(uint8_t *Buf, uint32_t Len);
		void Print(uint8_t *data, uint32_t size);

        static void TaskThread(void *argument);

		void TransmitTxFrame(); //TODO de-commission this one

	    void TransmitDataFrame(CMD_TYPE cmd,
	                            uint32_t deviceAddress,
	                            uint32_t data0,
	                            uint32_t data1,
	                            uint32_t data2,
	                            uint32_t data3);

	    void Init();

		void Filters(uint16_t len);

		void SendPingResponse();

		void RegisterCallback(GenericCallback* callBack) override;

		DataFrame dataFrame;
		DataFrame rxData;
		DataFrame txData;

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

		uint8_t rxByte;
		uint32_t size;

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
