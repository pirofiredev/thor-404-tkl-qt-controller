# Genesis Thor 404 TKL Linux Controller (`everything` branch)

Comprehensive Linux Qt6 & C++20 controller for Genesis Thor 404 TKL keyboard (and compatible boards).

## Features
- **Hardware lighting control**: 16 baked firmware effects (Wave, Breathing, Neon, Spectrum, Stars, Raindrops, Sine Wave, etc.), static RGB picker, speed/brightness/direction.
- **Hardware Profiles**: Onboard memory profile switching (Profiles 1–3) without local file bloat.
- **Direct Hardware Macros**: Write macro sequences directly into the keyboard's memory.
- **KDE Plasma Native Brightness Applet Integration**: Exposes virtual sysfs LED interface (`$XDG_RUNTIME_DIR/genesis_thor404::kbd_backlight.brightness`) so KDE's native slider, Fn keys, and OSD directly adjust keyboard brightness.
- **Home Assistant MQTT Discovery**: Auto-registers a native Light entity (`homeassistant/light/thor_404/config`) supporting state, brightness, RGB color, and effects.
- **JamesDSP-inspired UI**: Clean layout inheriting KDE Plasma / Breeze theme with card tabs.
- **Monochrome Adaptive System Tray**: Tray icon automatically adapts to light and dark desktop themes, minimizes on close (X button), and offers quick lighting actions.

## Building
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Running
```bash
# Launch GUI (runs in background with system tray)
./build/thor-404-controller

# Headless CLI commands
./build/thor-404-controller --list-devices
./build/thor-404-controller --set-static 0 255 0           # Static Green
./build/thor-404-controller --set-effect breathing --speed 8
./build/thor-404-controller --profile 0                    # Switch hardware profile
./build/thor-404-controller --test-framing                # Verify packet framing
```
