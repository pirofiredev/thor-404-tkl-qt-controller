# Genesis Thor 404 TKL Linux Controller (`everything` branch)

Comprehensive Linux Qt6 & C++20 controller for Genesis Thor 404 TKL keyboard (and compatible boards).

## Features
- **Interactive TKL ISO-88 Keyboard Visualizer**: Real-time interactive visualizer matching the exact physical layout. Drag or click keys to paint individual LED colors, or use quick-group shortcuts (WASD, Arrows, Fill All).
- **Hardware Lighting Control**: Full firmware effect suite (Static, Breathing, Neon, Spectrum, Stars, Raindrops, Sine Wave, Circle, Shuttle, Shockwave, Explosion, etc.) with custom RGB swatches, speed, brightness, and direction controls.
- **Customize (Per-Key RGB)**: Direct support for per-key custom lighting packet matrix framing.
- **KDE Plasma Native Brightness Applet Integration**: Exposes a virtual sysfs LED interface (`$XDG_RUNTIME_DIR/genesis_thor404::kbd_backlight.brightness`) so KDE's native slider, Fn keys, and OSD directly adjust keyboard brightness.
- **Home Assistant MQTT Discovery**: Auto-registers a native Light entity (`homeassistant/light/thor_404/config`) supporting state, brightness, RGB color, and effects without third-party daemons.
- **JamesDSP-inspired UI**: Clean native Qt Widgets layout inheriting KDE Plasma / Breeze theme palettes.
- **Monochrome Adaptive System Tray**: Tray icon automatically adapts to light and dark desktop themes, minimizes on close (X button), and offers customizable quick lighting actions + instant Toggle Off/On.

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
./build/thor-404-controller --test-framing                # Verify packet framing
```
