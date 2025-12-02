#ifndef USB_VIRTUAL_COM_HPP
#define USB_VIRTUAL_COM_HPP

#include "IUart.hpp"

extern "C"
{
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
}

namespace HardwareLayer
{
	class UsbVirtualCom
	: public Common::IUart
	{
	public:
		UsbVirtualCom();
		void Init();
		void Transmit(uint8_t *data, uint32_t size);
		void RegisterOnReceiveCallback(Callback* callBack) override;

		IUart::Callback* callbackHandle;
	};
}
#endif // USB_VIRTUAL_COM_HPP
