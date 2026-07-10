#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <ICallback.hpp>
#include "IUart.hpp"
#include "Crc16.hpp"
#include "SystemData.hpp"
#include "UserInterface.hpp"
#ifndef PICCOLO_GUI
#include "FdcanDriver.hpp"
#endif
#include "LinkLayer.hpp"

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
	    CURR_2,
		MOTION_POS_COMMAND,
		MOTION_SPEED_COMMAND,
		MOTION_TORQUE_COMMAND
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

	struct __attribute__((packed)) Payload
	{
		uint8_t payload[8];
	};

	struct __attribute__((packed)) DataFrameCanBridge
	{
		CMD_TYPE cmd;
		uint8_t address;
		Payload payload;
		uint16_t checksum;
	};

	struct __attribute__((packed)) RealTimeCommand
	{
		CMD_TYPE cmd;
		uint8_t padding[3];
		uint8_t data[4];
	};

	class Communication
	: public Common::ICallback
	{
	public:
#ifndef PICCOLO_GUI
		Communication(Common::IUart& uartRef, Common::IUart& rs485Ref, HardwareLayer::FdCanDriver &canBusRef, Common::SystemData &systemDataRef, UserInterface& userInterfaceRef);
#else
		Communication(Common::IUart& uartRef, Common::IUart& rs485Ref, Common::SystemData &systemDataRef, UserInterface& userInterfaceRef);
#endif
		void OnUsbCdcReceive(uint8_t *Buf, uint32_t Len);
		void OnRs485Receive(uint8_t *Buf, uint32_t Len);
		void Print(uint8_t *data, uint32_t size);

        static void TaskThread(void *argument);

		void TransmitTxFrame(); //TODO de-commission this one

		void NotifySystemData(AppLayer::CMD_TYPE cmd, uint8_t* data);

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
		
#ifndef PICCOLO_GUI
		QueueHandle_t remoteCommand = NULL;
#endif

		Common::SerialFrame serialFrameRx;
		Common::SerialFrame serialFrameTx;	

		DataFrame dataFrame;
		DataFrame rxData;
		DataFrame txData;
		DataFrameCanBridge canOverSerialData;

		RealTimeCommand realtimeCommand;

	private:
		Common::IUart &usbCdc;
		Common::IUart &rs485;
#ifndef PICCOLO_GUI
		HardwareLayer::FdCanDriver &canBus;

		struct CanCallbackAdapter : public Common::ICallback::GenericCallback
		{
			Communication &parent;
			CanCallbackAdapter(Communication &p) : parent(p) {}
			void OnReceive(uint8_t* Buf, uint32_t Len, void* instance) override { parent.OnCanReceive(Buf, Len); }
			void OnCallback(uint8_t arg) override { (void)arg; }
		} canCallbackAdapter;
#endif

		bool isDataReceived;
		INTERFACE interface = INTERFACE::NONE;

		Common::SystemData& systemData;
		Common::ICallback::GenericCallback* callbackHandle;
		UserInterface& userInterface;

#ifndef PICCOLO_GUI
		void OnCanReceive(uint8_t* Buf, uint32_t Len);
#endif

		Common::UartReceiveAdapter<Communication> usbCdcCallback;
		Common::UartReceiveAdapter<Communication> rs485Callback;

		void ProcessFrame(uint8_t *Buf, uint32_t Len);
	};
}
#endif // COMMUNICATION_HPP
