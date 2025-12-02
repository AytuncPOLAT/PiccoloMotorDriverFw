#include "UsbVirtualCom.hpp"

using namespace HardwareLayer;

UsbVirtualCom* global_usbComPtr;

UsbVirtualCom::UsbVirtualCom()
{
	global_usbComPtr = this;
}

void UsbVirtualCom::Init()
{
	MX_USB_DEVICE_Init();
}

void UsbVirtualCom::Transmit(uint8_t *data, uint32_t size)
{
	CDC_Transmit_HS(data, size);
}

void USB_CDC_RxHandler(uint8_t *Buf, uint32_t Len)
{
	global_usbComPtr->callbackHandle->OnReceiveCallback(Buf, Len);
}

void UsbVirtualCom::RegisterOnReceiveCallback(Common::IUart::Callback* callback)
{
	callbackHandle = callback;
}
