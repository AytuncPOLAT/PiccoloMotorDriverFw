#ifndef SYSTEM_DATA_CONTROLLER_HPP
#define SYSTEM_DATA_CONTROLLER_HPP

#include <ICallback.hpp>
#include "SystemData.hpp"
#include "Communication.hpp"
#include "FlashMemoryController.hpp"
#include "UserInterface.hpp"

namespace AppLayer
{
	class SystemDataController
	: public Common::ICallback::GenericCallback
	{
	public:
		SystemDataController(Common::SystemData& systemDataRef,
							 Communication& communicationRef,
							 HardwareLayer::FlashStorage& storageControllerRef,
							 UserInterface& userInterfaceRef);

	private:
		bool CheckIfConfigBlank();
		bool LoadSystemDataFromStorage();
		void OnCallback(uint8_t arg) override;
		void DataReadResponse(Common::PROPERTY property);

		void WriteToRam(Common::PROPERTY property, uint32_t newValue);

		Common::SystemData& systemData;
		Communication& communication;
		HardwareLayer::FlashStorage& storageController;
		UserInterface& userInterface;
	};
}

#endif // SYSTEM_DATA_CONTROLLER_HPP
