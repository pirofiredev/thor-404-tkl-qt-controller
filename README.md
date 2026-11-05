# Genesis Thor 404 TKL Linux Controller (`everything` branch)

Comprehensive Linux Qt6 & C++20 controller for Genesis Thor 404 TKL keyboard (and compatible boards).

## Features
- **Interactive TKL ISO-88 Keyboard Visualizer**: Real-time interactive visualizer matching the exact physical layout. Drag or click keys to paint individual LED colors, or use quick-group shortcuts (WASD, Arrows, Fill All).
- **Contextual Lighting Controls**: Direction, speed, rainbow toggles, and matrix painting controls automatically hide when in **Static** or non-applicable modes for a clean, distraction-free UI.
- **Genesis SVG App & Tray Icon**: Styled Genesis emblem with full Wayland desktop taskbar support, desktop entry integration, and theme-adaptive tray icon.
- **System Startup & Autostart**: GUI option to automatically launch on system login in the background.
- **Home Assistant MQTT Discovery**: Auto-registers a native Light entity (`homeassistant/light/thor_404/config`) with persistent auto-reconnect on startup.
- **JamesDSP-inspired UI**: Clean native Qt Widgets layout inheriting KDE Plasma / Breeze theme palettes.

---

## Installation via `.deb` (Recommended)

Download the `.deb` release package from GitHub Releases or build your own:

```bash
sudo apt install ./build/thor-404-controller-1.0.0-Linux.deb
```

When installed via `apt`:
- Installs the binary to `/usr/bin/thor-404-controller`.
- Automatically installs `/lib/udev/rules.d/99-genesis-thor.rules` so you never get permission errors.
- Automatically generates the application launcher and taskbar icon (`thor-404-controller.desktop`).

---

## Home Assistant & MQTT Broker Setup

### 1. Set Up Mosquitto MQTT Broker (on your PC)
Option 1: Native Mosquitto via APT (Easiest for Ubuntu/Debian)

```bash
# 1. Install Mosquitto and CLI tools
sudo apt update
sudo apt install -y mosquitto mosquitto-clients

# 2. Configure local listener (allow connections without passwords for local LAN/PC):
sudo tee /etc/mosquitto/conf.d/local.conf << 'EOF'
listener 1883
allow_anonymous true
EOF

# 3. Enable and start the service
sudo systemctl enable mosquitto
sudo systemctl restart mosquitto

# 4. Verify it is running
systemctl status mosquitto
```

### 2. Connect in Home Assistant
1. Find your machine's IP address:
   ```bash
   ip -4 addr show scope global
   ```
2. In Home Assistant: Go to **Settings** $\rightarrow$ **Devices & Services** $\rightarrow$ Click **+ Add Integration** $\rightarrow$ Search for **MQTT**.
3. Select standard **MQTT** (not JSON):
   - **Broker**: Enter your PC's LAN IP (from `ip a`).
   - **Port**: `1883`.
   - Leave Username and Password empty (if using anonymous `local.conf`), and click **Submit**.
4. In Thor 404 Controller:
   - Go to the **Integrations** tab.
   - Enter `127.0.0.1` and Port `1883`.
   - Check **Auto-connect to MQTT on startup**.
   - Click **Connect to MQTT Broker**.
5. Done! The keyboard will instantly register as a native Light entity on your Home Assistant dashboard with On/Off, Brightness, Color Wheel, and Effect controls.

---

## Building From Source
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Optional: Generate deb package
cd build && cpack -G DEB
```

## Running
```bash
# Launch GUI (runs in background with system tray)
thor-404-controller

# Headless CLI commands
thor-404-controller --list-devices
thor-404-controller --set-static 0 255 0           # Static Green
thor-404-controller --set-effect breathing --speed 8
thor-404-controller --test-framing                # Verify packet framing
```
