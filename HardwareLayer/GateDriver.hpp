#ifndef GATE_DRIVER_HPP
#define GATE_DRIVER_HPP

#include "BoardConfig/BoardConfig.h"

// The selected board profile determines which gate-driver implementation is used.
#if defined(GATE_DRIVER_DRV8353S)
#include "DRV8353S_SpiDriver.hpp"
namespace HardwareLayer
{
	using GateDriver = Drv8353sSpiDriver;
}
#else
#include "DRV8316R_SpiDriver.hpp"
namespace HardwareLayer
{
	using GateDriver = ::Drv8316rSpiDriver;
}
#endif

#endif // GATE_DRIVER_HPP
