# Genesis Thor 404 TKL RGB Controller (Linux)

Native Linux Qt6/C++20 desktop application to control RGB lighting on the Genesis Thor 404 TKL keyboard (and compatible Genesis boards). Built with Qt Widgets to automatically inherit the native KDE Plasma / Breeze dark theme without heavy stylesheets.

## Features
- Hardware support for Genesis Thor 404 TKL (`0x331A:0x501B` and `0x331A:0x501C`).
- Communication via `hidapi-hidraw` (no root required with udev rules).
- Connection status badge (Connected / Disconnected) with dynamic auto-detection.
- Preset lighting effects: Static, Breathing, Neon, Wave, Off.
- Color picker dialog (`QColorDialog`) with dynamic color preview swatch.
- Brightness (1–16), Speed (1–16), Direction (Right, Left, Up, Down), and Rainbow toggles.
- Handshake and finalize sequence with 8ms write pacing.
- Built-in assert-based packet framing validation (`--test-framing`).

## Setup udev Rules (Non-root USB Access)
Copy `99-genesis-thor.rules` to `/etc/udev/rules.d/` and reload udev:
```bash
sudo cp 99-genesis-thor.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger

## Build Instructions
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Running
```bash
# Launch GUI
./build/thor-404-controller

# CLI commands (headless, no GUI)
./build/thor-404-controller --list-devices
./build/thor-404-controller --set-static 255 0 0           # Pure Red
./build/thor-404-controller --set-effect wave --speed 12   # Wave effect
./build/thor-404-controller --set-effect off               # Turn lighting off
./build/thor-404-controller --debug                        # Verbose packet logging

# Run packet framing verification tests
./build/thor-404-controller --test-framing
# or standalone test runner:
./build/test-framing
```
