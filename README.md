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

## Motor control startup
Motor initialization applies the configuration loaded from flash (or the defaults
on first boot) to all PID controllers before creating the motor control task.
No PID write from PiccoloGui is required after a reboot. GUI configuration writes
continue to refresh the controllers at runtime; save changes to device flash if
they should persist across reboots.

### Torque direction
`inverseTorque` defaults to off. When enabled, it reverses the torque command in
torque, speed, and position modes; electrical-angle mode is unchanged.
In PiccoloGui, open **Motor Parameters**, toggle **Inverse Torque**, and click
**W** to apply it. **R** or **Read All** reads the current value; **Store** saves
the device configuration to flash so it survives a reboot.

The flag uses a previously reserved configuration word, preserving the existing
flash layout and property IDs. Older reserved values other than `1` load as off.

### DRV8353S gate driver configuration
PiccoloGui's **Gate Driver (DRV8353S)** section exposes high-/low-side source and
sink current, peak drive time, and CSA gain (5, 10, 20, or 40 V/V). **W** applies
each setting immediately and verifies the device response; **R** / **Read All**
read the configuration. **Store** persists all settings across reboots; JSON
save/load includes them.

Defaults remain 50 mA source / 100 mA sink on both sides, 4000 ns drive time,
and 20 V/V CSA gain. Existing flash settings are retained; a version marker in
reserved space initializes only the newly implemented gate settings on upgrade.
Current conversion scales relative to the existing 20 V/V calibration.

Status codes: 0 = OK, 1 = unsupported IC, 2 = invalid value, 3 = SPI/readback
failure. Failed SPI writes do not commit configuration and disarm the motor.
No disarm requirement is enforced for setting changes; changing strength or gain
while driving can disturb motor control. DRV8316R configuration is not supported.

## Project layout
- `AppLayer/` — application logic and control loops.
- `HardwareLayer/` — hardware abstraction and peripheral drivers.
- `PiccoloGui/` — host GUI and tools.

## License
This project is covered by the terms in the `LICENSE` file.

Questions or issues: please open an issue in the repository.
