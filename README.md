# Piccolo Motor Driver Firmware

Compact firmware for the Piccolo motor driver platform — STM32H7-based motor control, sensor drivers, and a small host GUI.

## Quick overview
- Core firmware: `AppLayer/`, `HardwareLayer/` — motor control, PID, telemetry.
- Drivers: `Drivers/` and `HardwareLayer/` (ADC, SPI, encoders, FDCAN, USB).
- GUI: `PiccoloGui/` — desktop interface for monitoring and control.

## Quickstart (Linux)
Prerequisites: ARM toolchain (gcc-arm-none-eabi), CMake, build tools.

1. Configure & build (using CMake presets):

```
cd /path/to/PiccoloMotorDriverFw
cmake --preset Release
cmake --build --preset Release
```

2. Flash the device

```
./flash.sh
```

3. Run the GUI (optional):

```
cd PiccoloGui
./build.sh
```

Notes:
- `flash.sh` wraps the platform-specific flashing steps; run with appropriate permissions.
- See `CMakePresets.json` for build configuration and targets.

## Project layout
- `AppLayer/` — application logic and control loops.
- `HardwareLayer/` — hardware abstraction and peripheral drivers.
- `PiccoloGui/` — host GUI and tools.

## License
This project is covered by the terms in the `LICENSE` file.

Questions or issues: please open an issue in the repository.
