#ifndef SYSTEM_DATA_CONTROLLER_HPP
#define SYSTEM_DATA_CONTROLLER_HPP

#include <ICallback.hpp>
#include "SystemData.hpp"
#include "Communication.hpp"
#include "FlashMemoryController.hpp"
#include "UserInterface.hpp"
#include "DRV8316R_SpiDriver.hpp"

namespace AppLayer
{
	enum class State
	{
		IDLE = 0,
		FLASH_WRITE
	};

	class SystemDataController
	: public Common::ICallback::GenericCallback
	{
	public:
		SystemDataController(Common::SystemData& systemDataRef,
							 Communication& communicationRef,
							 HardwareLayer::FlashStorage& storageControllerRef,
							 UserInterface& userInterfaceRef,
							 Drv8316rSpiDriver& drvRef);
		void Init();
	private:
		bool CheckIfConfigBlank();
		bool LoadSystemDataFromStorage();
		void OnCallback(uint8_t arg) override;
		void DataReadResponse(Common::PROPERTY property);

		void WriteToRam(Common::PROPERTY property, uint32_t newValue);

		BaseType_t taskHandle;
		static void TaskThread(void *argument);

		Common::SystemData& systemData;
		Communication& communication;
		HardwareLayer::FlashStorage& storageController;
		UserInterface& userInterface;
		Drv8316rSpiDriver& drv;

		State state = State::IDLE;
	};
}

#endif // SYSTEM_DATA_CONTROLLER_HPP
