#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP

namespace Common
{
	enum class ErrorType : uint8_t
	{
		OK = 0,
		MOTOR_OVERCURRENT = 1,
		MOTOR_OVERTEMP = 2,
		UNDER_VOLTAGE = 3,
		OVER_VOLTAGE = 4,
		CONTROLLER_MAX_ERROR = 5,
		CPU_MAX_UTILIZATION = 6,
		FLASHMEM
	};
}

#endif
