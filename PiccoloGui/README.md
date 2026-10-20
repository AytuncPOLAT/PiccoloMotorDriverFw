# PiccoloGui - Servo Driver Configuration Tool

This project is a Dear ImGui desktop GUI starter for configuring a servo motor driver.

## Features in this starter

- Top-left serial connection panel
- Left-side configuration panel with 5 categories of input fields
- Right-side real-time plotting panel using ImPlot
- Bottom communication log panel
- Windows serial support for COM ports
- **Inverse Torque** checkbox under **Motor Parameters**: **R** reads the flag,
  **W** applies it to device RAM, and **Store** saves it to device flash.
  It defaults to off (`0`); on (`1`) reverses the torque command in torque,
  speed, and position modes. Electrical-angle mode is unchanged.
  **Read All** and JSON **Save/Load Configuration** include the flag.

## Gate Driver (DRV8353S)

- Select high-/low-side source and sink current (mA), drive time (ns), and CSA
  gain (V/V) from the dropdowns. Read the **Gate Driver IC** field to check the
  connected firmware variant (8353 or 8316).
- **W** applies and verifies a setting; **R** and **Read All** read device
  configuration. **Store** saves to device flash. JSON save/load includes these
  fields but loading JSON does not write to the device.
- Gate configuration status: 0 = OK, 1 = unsupported driver, 2 = invalid value,
  3 = SPI/readback failure. Write failures are reported in the communication log.
- Defaults: 50 mA source and 100 mA sink on both sides, 4000 ns, 20 V/V gain.
- Changes apply immediately, without requiring disarm; avoid changing gate drive
  or current-feedback gain while driving. DRV8316R writes are rejected.

## Build (Windows, Visual Studio generator)

```powershell
cmake -S . -B build_windows -G "Visual Studio 16 2019" -A x64
cmake --build build_windows --config Release --target PiccoloGui --parallel
```

## Run

```powershell
.\build_windows\Release\PiccoloGui.exe
```

## Telemetry format expected from firmware

Incoming lines are parsed as CSV:

```text
speed_rps,current_a,position_deg
```

Example:

```text
12.3,1.8,45.0
```

## Notes

- `Apply All Settings` currently sends `CFG:APPLY` as a placeholder command.
- Extend serialization to map each field to your firmware protocol.
