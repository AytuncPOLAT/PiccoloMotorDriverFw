#ifndef SIMPLE_LOGGER_HPP
#define SIMPLE_LOGGER_HPP

#include "IUart.hpp"
#include "UsbVirtualCom.hpp"

namespace AppLayer
{
	class SimpleLogger
	: public Common::IUart::Callback
	{
	public:
		SimpleLogger(Common::IUart &uartRef);
		void OnReceiveCallback(uint8_t *Buf, uint32_t Len) override;

	private:
		Common::IUart &uart;
	};
}

#endif
