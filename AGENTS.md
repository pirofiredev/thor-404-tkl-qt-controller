# Genesis Thor 404 TKL Linux Controller - Agent Guidelines

## Hardware & USB HID Specifications
- **Vendor ID**: `0x331A`
- **Product IDs**:
  - `0x501B` (Thor 404 Variant A)
  - `0x501C` (Thor 404 Variant B)
- **USB Endpoints & Routing**:
  - The keyboard exposes multiple interfaces.
  - **Interface 0** (`/dev/hidraw5` or similar) is the ONLY interface that accepts the 64-byte `Consumer` Feature Report collection (Report ID `0x00`, 65 bytes total buffer).
  - **Interface 1** is input-only. Attempting to send feature reports to Interface 1 will fail with `ioctl: Broken pipe (EPIPE)`. Always target Interface 0.
- **udev Permissions**:
  - `99-genesis-thor.rules` grants non-root access to both product IDs under `hidraw` and `usb`.

## Packet Protocol & Write Sequence
To successfully apply lighting changes to the keyboard's internal controller, the complete sequence must be sent with ~10ms sleep between writes:

1. **Handshake** (3 packets, 65 bytes each, zero-padded):
   - `00 04 02 ...`
   - `00 04 19 ...`
   - `00 04 13 00 00 00 00 00 00 12 ...`
2. **Common Prefix Configuration** (8 packets):
   - Initializes firmware display buffers (see `makeCommonPrefixPackets()` in `src/devicemanager.cpp`).
3. **Key Frame Rows** (9 packets):
   - 16 DWORD blocks per row starting with byte `0x80` (`80 00 00 00`).
4. **Effect Packet** (1 packet):
   - Byte 0: `0x00` (Report ID)
   - Byte 1: Effect ID (`0x00` Off, `0x01` Static, `0x07` Breathing, `0x08` Neon, `0x12` Wave, etc.)
   - Byte 2: Red (`0-255`)
   - Byte 3: Green (`0-255`)
   - Byte 4: Blue (`0-255`)
   - Byte 9: Multicolor flag (`0x00` custom color, `0x01` rainbow)
   - Byte 10: Brightness (`1-16`)
   - Byte 11: Speed (`1-16`)
   - Byte 12: Direction (`0=Right, 1=Left, 2=Up, 3=Down`)
   - Byte 15: `0xAA`
   - Byte 16: `0x55`
5. **Finalize Packets** (2 packets):
   - `00 04 02 ...`
   - `00 04 F0 ...`

## Project Structure
- `src/devicemanager.h` / `src/devicemanager.cpp`: USB communication via `hidapi-hidraw`, device enumeration, packet generation, and feature report transmission.
- `src/mainwindow.h` / `src/mainwindow.cpp`: Qt6 Widgets UI designed to cleanly inherit the native desktop theme (KDE Plasma / Breeze dark) without heavy custom CSS.
- `src/main.cpp`: Entry point supporting both GUI launch and headless CLI operations (`--set-static`, `--set-effect`, `--list-devices`, `--debug`).
- `tests/test_framing.cpp`: Assertion check for packet buffer layouts and byte offsets.

## Build and Test
```bash
# Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run packet framing verification test
./build/test-framing

# Run headless CLI commands
./build/thor-404-controller --list-devices
./build/thor-404-controller --set-static 255 0 0 --debug
./build/thor-404-controller --set-effect breathing --speed 8
```

## Nice-to-have Roadmap (Planned Features)
1. **Profiles**:
   - Hardware-backed profile switching.
   - Stored in keyboard MCU memory via one-shot feature reports.
2. **Macros & Key Remapping**:
   - Hardware key assignment write payloads.
   - Custom per-key RGB layout (Effect `0x14`).
