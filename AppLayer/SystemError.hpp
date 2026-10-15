#ifndef SYSTEM_ERROR_HPP
#define SYSTEM_ERROR_HPP

#include "ErrorHandler.hpp"

class UserInterface;

namespace AppLayer
{
class MotorControl;

class SystemError
{
public:
	SystemError(MotorControl& motorControlRef, UserInterface& userInterfaceRef);

	void SetError(Common::ErrorType error);
	void ClearError();
	bool HasError() const;
	Common::ErrorType GetError() const;

private:
	MotorControl& motorControl;
	UserInterface& userInterface;
	Common::ErrorType currentError = Common::ErrorType::OK;
	bool hasError = false;
};

} // namespace AppLayer

#endif // SYSTEM_ERROR_HPP

