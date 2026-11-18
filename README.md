# Piccolo Motor Driver Firmware

Compact firmware for the Piccolo motor driver platform — STM32H7-based motor control, sensor drivers, and a small host GUI.

## Quick overview
- Core firmware: `AppLayer/`, `HardwareLayer/` — motor control, PID, telemetry.
- Drivers: `Drivers/` and `HardwareLayer/` (ADC, SPI, encoders, FDCAN, USB).
- GUI: `PiccoloGui/` — desktop interface for monitoring and control.

## Quickstart (Linux)
Prerequisites: CMake 3.22 or newer, Ninja, and the ARM toolchain (gcc-arm-none-eabi).

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

To select the Piccolo2 PCB profile explicitly:

```
cmake --preset Release-Piccolo2
cmake --build --preset Release-Piccolo2
```

The build directory contains both `PiccoloMotorDriver.elf` and
`PiccoloMotorDriver.hex`.

3. Run the GUI (optional):

```
cd PiccoloGui
./build.sh
```

Notes:
- `flash.sh` wraps the platform-specific flashing steps; run with appropriate permissions.
- See `CMakePresets.json` for build configuration and targets.
- Piccolo2 uses the DRV8353S, 5 mΩ shunts, and the 47 kΩ / 1 kΩ supply divider
  from the KiCad schematic. PiccoloZero uses the DRV8316R but cannot be built
  until its pinout and electrical values are defined.

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

## Weight-to-torque GUI (Windows)

Run from the repository root with Python 3:

```
py -3 -m pip install -r scripts\requirements.txt
py -3 scripts\weight_torque_gui.py
```

The weight sensor port defaults to COM36; choose the motor driver's port
separately. Both baud rates default to 115200 and can be changed in the GUI.
The sensor should send comma-separated ASCII lines; the third field is used as
the weight in grams and may include a `g`/`grams` suffix. The slider scales grams into the
integer torque command sent using the firmware's serial frame format. Starting
output selects torque control but does not arm the motor. Stopping output or
losing fresh weight data sends a zero torque command.

## Project layout
- `AppLayer/` — application logic and control loops.
- `HardwareLayer/` — hardware abstraction and peripheral drivers.
- `PiccoloGui/` — host GUI and tools.
- `scripts/` — utility scripts, including the weight-to-torque GUI.

## License
This project is covered by the terms in the `LICENSE` file.

Questions or issues: please open an issue in the repository.
