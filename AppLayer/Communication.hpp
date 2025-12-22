#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <ICallback.hpp>
#include "IUart.hpp"
#include "Crc16.hpp"
#include "SystemData.hpp"

namespace AppLayer
{
	enum class CMD_TYPE : uint8_t
	{
		READ_FROM_DEV = 0,
		WRITE_TO_DEVICE = 1
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
		Communication(Common::IUart& uartRef, Common::SystemData &systemDataRef);
		void OnReceiveCallback(uint8_t *Buf, uint32_t Len) override;
		void Print(uint8_t *data, uint32_t size);
		uint8_t ReadByte();
		void Plot(uint32_t);
		bool GetRxStatus();
		uint32_t GetPayload();
		void RegisterCallback(GenericCallback* callBack) override;

		DataFrame dataFrame;
		DataFrame rxData;
		DataFrame txData;

	private:
		Common::IUart &uart;
		uint8_t rxByte;
		uint32_t size;

		bool isDataReceived;

		Common::SystemData& systemData;
		ICallback::GenericCallback* callbackHandle;
	};
}
#endif // COMMUNICATION_HPP
