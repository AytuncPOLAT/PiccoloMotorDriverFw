#ifndef SYSTEM_DATA_CONTROLLER_HPP
#define SYSTEM_DATA_CONTROLLER_HPP

#include "SystemData.hpp"
#include "Communication.hpp"
#include "FlashMemoryController.hpp"

namespace AppLayer
{
	class SystemDataController
	{
	public:
		SystemDataController(Common::SystemData& systemDataRef,
							 Communication& communicationRef,
							 HardwareLayer::FlashStorage& storageControllerRef);

	private:
		bool CheckIfConfigBlank();

		Common::SystemData& systemData;
		Communication& communication;
		HardwareLayer::FlashStorage& storageController;
	};
}

#endif // SYSTEM_DATA_CONTROLLER_HPP
