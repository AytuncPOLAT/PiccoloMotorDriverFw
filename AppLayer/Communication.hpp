#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <ICallback.hpp>
#include "IUart.hpp"
#include "Crc16.hpp"
#include "SystemData.hpp"
#include "UserInterface.hpp"

namespace AppLayer
{
	enum class INTERFACE : uint8_t
	{
		NONE = 0,
		USB_CDC,
		RS485
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
	: public Common::IUart::Callback
	, public Common::ICallback
	{
	public:
		Communication(Common::IUart& uartRef, Common::IUart& rs485Ref, Common::SystemData &systemDataRef, UserInterface& userInterfaceRef);
		void OnReceiveCallback(uint8_t *Buf, uint32_t Len, void* instance) override;
		void Print(uint8_t *data, uint32_t size);

		void TransmitTxFrame(); //TODO de-commission this one

	    void TransmitDataFrame(CMD_TYPE cmd,
	                            uint32_t deviceAddress,
	                            uint32_t data0,
	                            uint32_t data1,
	                            uint32_t data2,
	                            uint32_t data3);

		void Filters(uint16_t len);

		void SendPingResponse();

		void RegisterCallback(GenericCallback* callBack) override;

		DataFrame dataFrame;
		DataFrame rxData;
		DataFrame txData;

	private:
		Common::IUart &usbCdc;
		Common::IUart &rs485;

		uint8_t rxByte;
		uint32_t size;

		bool isDataReceived;
		INTERFACE interface = INTERFACE::NONE;

		Common::SystemData& systemData;
		ICallback::GenericCallback* callbackHandle;
		UserInterface& userInterface;
	};
}
#endif // COMMUNICATION_HPP
