#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <thread>
#include <hidapi.h>

namespace thor {

constexpr uint16_t VENDOR_ID = 0x331A;
constexpr uint16_t PID_VARIANT_A = 0x501B;
constexpr uint16_t PID_VARIANT_B = 0x501C;

constexpr size_t REPORT_SIZE = 65; // 1 byte report ID (0x00) + 64 bytes payload

enum class EffectId : uint8_t {
    Off = 0x00,
    Static = 0x01,
    SingleOn = 0x02,
    SingleOff = 0x03,
    Stars = 0x04,
    Raindrops = 0x05,
    Colourful = 0x06,
    Breathing = 0x07,
    Neon = 0x08,
    Spectrum = 0x09,
    Rainbow = 0x0A,
    Prismo = 0x0B,
    Circle = 0x0C,
    EscapeBeam = 0x0D,
    Shockwave = 0x0E,
    Explosion = 0x0F,
    Escape = 0x10,
    SineWave = 0x11,
    Wave = 0x12,
    Shuttle = 0x13
};

enum class Direction : uint8_t {
    Right = 0,
    Left = 1,
    Up = 2,
    Down = 3
};

struct EffectConfig {
    EffectId effect{EffectId::Static};
    uint8_t r{255};
    uint8_t g{255};
    uint8_t b{255};
    bool multicolor{false};
    uint8_t brightness{16}; // 1-16
    uint8_t speed{16};      // 1-16
    Direction direction{Direction::Right};
};

struct DeviceInfo {
    std::string path;
    uint16_t vendorId{0};
    uint16_t productId{0};
    int interfaceNumber{-1};
    std::string productName;
    std::string serialNumber;
};

// Pure packet construction functions (unit testable without USB)
using PacketBuffer = std::array<uint8_t, REPORT_SIZE>;

PacketBuffer makeHandshakePacket1();
PacketBuffer makeHandshakePacket2();
PacketBuffer makeHandshakePacket3();
std::vector<PacketBuffer> makeCommonPrefixPackets();
std::vector<PacketBuffer> makeCommonFramePackets();
PacketBuffer makeEffectPacket(const EffectConfig& config);
PacketBuffer makeFinalizePacket1();
PacketBuffer makeFinalizePacket2();

class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    std::vector<DeviceInfo> enumerateDevices() const;
    bool isConnected() const;
    bool openDevice(uint16_t targetPid = 0);
    void closeDevice();

    bool applyEffect(const EffectConfig& config, std::string* errorMessage = nullptr);
    bool setProfile(uint8_t profileIndex, std::string* errorMessage = nullptr);
    bool blinkProfileKey(uint8_t profileIndex, int blinkCount = 3);
    bool assignKeyMacro(uint8_t keyIndex, const std::vector<uint8_t>& scancodes, std::string* errorMessage = nullptr);

    void setDebug(bool enable) { m_debug = enable; }
    bool isDebug() const { return m_debug; }

    std::optional<DeviceInfo> currentDevice() const { return m_currentInfo; }

private:
    bool sendFeatureReport(const PacketBuffer& buffer);

    hid_device* m_handle{nullptr};
    std::optional<DeviceInfo> m_currentInfo;
    bool m_debug{false};
};

} // namespace thor
