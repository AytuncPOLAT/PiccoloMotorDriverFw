# PiccoloGui - Servo Driver Configuration Tool

This project is a Dear ImGui desktop GUI starter for configuring a servo motor driver.

## Features in this starter

- Top-left serial connection panel
- Left-side configuration panel with 5 categories of input fields
- Right-side real-time plotting panel using ImPlot
- Bottom communication log panel
- Windows serial support for COM ports

## Build (Windows, Visual Studio generator)

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Run

```powershell
.\build\Release\PiccoloGui.exe
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
