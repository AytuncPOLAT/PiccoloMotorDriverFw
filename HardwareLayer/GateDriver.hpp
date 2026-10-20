#ifndef GATE_DRIVER_HPP
#define GATE_DRIVER_HPP

// Compile-time gate driver IC selection (board variant). Set via CMake: -DGATE_DRIVER=DRV8353S
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
