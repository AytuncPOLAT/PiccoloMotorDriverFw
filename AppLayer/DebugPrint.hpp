#ifndef DEBUG_PRINT_HPP
#define DEBUG_PRINT_HPP

#include "IUart.hpp"

extern "C"
{
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
}

namespace AppLayer
{
	class DebugPrint
	{
	public:
		DebugPrint(const DebugPrint &c) = delete;
		DebugPrint&operator=(const DebugPrint &c) = delete;

		static DebugPrint&
		Instance();

		void
		Print(uint8_t *data, uint32_t len);

	private:
		DebugPrint();

	};
}
#endif //DEBUG_PRINT_HPP
