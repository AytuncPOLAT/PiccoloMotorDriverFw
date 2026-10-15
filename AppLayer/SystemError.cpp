#include "SystemError.hpp"
#include "MotorControl.hpp"
#include "UserInterface.hpp"

using namespace AppLayer;

SystemError::SystemError(MotorControl& motorControlRef, UserInterface& userInterfaceRef)
	: motorControl(motorControlRef)
	, userInterface(userInterfaceRef)
	, currentError(Common::ErrorType::OK)
	, hasError(false)
{
}

void SystemError::SetError(Common::ErrorType error)
{
	currentError = error;
	hasError = true;

	// In error mode, motor should be disarmed and red LED blinks at 2hz
	motorControl.SetArmed(false);
	userInterface.SetUiState(UiState::Error);
}

void SystemError::ClearError()
{
	currentError = Common::ErrorType::OK;
	hasError = false;
}

bool SystemError::HasError() const
{
	return hasError;
}

Common::ErrorType SystemError::GetError() const
{
	return currentError;
}

