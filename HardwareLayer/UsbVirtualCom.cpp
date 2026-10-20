#include "UsbVirtualCom.hpp"

using namespace HardwareLayer;

UsbVirtualCom* global_usbComPtr;

UsbVirtualCom::UsbVirtualCom()
: callbackHandle(nullptr)
{
	global_usbComPtr = this;
}

void* UsbVirtualCom::GetInstance()
{
	return this;
}

void UsbVirtualCom::Init()
{
	MX_USB_DEVICE_Init();
}

extern USBD_HandleTypeDef hUsbDeviceHS;

uint8_t UsbVirtualCom::Transmit(uint8_t *data, uint32_t size)
{
	USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassData;
	static uint8_t s_txBuf[64];
	if (size > sizeof(s_txBuf))
	{
		size = sizeof(s_txBuf);
	}

	uint32_t timeout = 100000;
	while (hcdc != nullptr && hcdc->TxState != 0 && --timeout > 0)
	{
	}

	memcpy(s_txBuf, data, size);
	CDC_Transmit_HS(s_txBuf, size);

	timeout = 100000;
	while (hcdc != nullptr && hcdc->TxState != 0 && --timeout > 0)
	{
	}

	return 0;
}

uint8_t UsbVirtualCom::Receive(uint8_t *data, uint32_t size)
{

}

void UsbVirtualCom::RegisterOnReceiveCallback(Common::IUart::Callback* callback)
{
	callbackHandle = callback;
}

void USB_CDC_RxHandler(uint8_t *Buf, uint32_t Len)
{
	if(global_usbComPtr->callbackHandle != nullptr)
	{
		global_usbComPtr->callbackHandle->OnReceiveCallback(Buf, Len, global_usbComPtr);
	}
}
