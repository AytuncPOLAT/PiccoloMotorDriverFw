#include "UsbVirtualCom.hpp"

using namespace HardwareLayer;

UsbVirtualCom* global_usbComPtr;

UsbVirtualCom::UsbVirtualCom()
: callbackHandle(nullptr)
{
	global_usbComPtr = this;
}

void UsbVirtualCom::Init()
{
	MX_USB_DEVICE_Init();
}

uint8_t UsbVirtualCom::Transmit(uint8_t *data, uint32_t size)
{
	CDC_Transmit_HS(data, size);
	return 0;
}

uint8_t UsbVirtualCom::Receive(uint8_t *data, uint32_t size)
{

}

void UsbVirtualCom::RegisterOnReceiveCallback(Common::IUart::Callback* callback)
{
	callbackHandle = callback;
	global_usbComPtr->callbackHandle->OnReceiveCallback(nullptr,0);
}

void USB_CDC_RxHandler(uint8_t *Buf, uint32_t Len)
{
	if(global_usbComPtr->callbackHandle != nullptr)
	{
		global_usbComPtr->callbackHandle->OnReceiveCallback(Buf, Len);
	}
}
