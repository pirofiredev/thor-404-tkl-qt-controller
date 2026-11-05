#include "devicemanager.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <iomanip>

namespace thor {

PacketBuffer makeHandshakePacket1() {
    PacketBuffer buf{};
    buf[0] = 0x00; // Report ID
    buf[1] = 0x04;
    buf[2] = 0x02;
    return buf;
}

PacketBuffer makeHandshakePacket2() {
    PacketBuffer buf{};
    buf[0] = 0x00; // Report ID
    buf[1] = 0x04;
    buf[2] = 0x19;
    return buf;
}

PacketBuffer makeHandshakePacket3() {
    PacketBuffer buf{};
    buf[0] = 0x00; // Report ID
    buf[1] = 0x04;
    buf[2] = 0x13;
    buf[9] = 0x12; // Byte 8 in payload (index 9 in buffer)
    return buf;
}

static PacketBuffer fromHexPayload(const char* hex) {
    PacketBuffer buf{};
    buf[0] = 0x00;
    size_t hexLen = std::strlen(hex);
    for (size_t i = 0; i < hexLen && (i / 2) < 64; i += 2) {
        char byteStr[3] = { hex[i], hex[i+1], '\0' };
        buf[1 + (i / 2)] = static_cast<uint8_t>(std::strtoul(byteStr, nullptr, 16));
    }
    return buf;
}

std::vector<PacketBuffer> makeCommonPrefixPackets() {
    return {
        fromHexPayload("01ff00000000000000100c000000aa55020000ff0000000001100c000000aa55030000ff0000000001100c000000aa55040000ff0000000001100c000000aa55"),
        fromHexPayload("050000ff0000000001100c000000aa55060000ff0000000000100c000000aa55075affff0000000000100c000000aa55080000ff0000000000100c000000aa55"),
        fromHexPayload("090000ff0000000001100c030000aa550a0000ff0000000001100c030000aa550b0000ff0000000001100c000000aa550c0000ff0000000001100c000000aa55"),
        fromHexPayload("0d0000ff0000000001100c000000aa550e0000ff0000000001100c000000aa550f0000ff0000000001100c000000aa55100000ff0000000001100c000000aa55"),
        fromHexPayload("110000ff0000000001100c000000aa55120000ff0000000001100c000000aa55130000ff0000000001100c000000aa558000000000000000001000000000aa55"),
        PacketBuffer{}, // zero payload
        PacketBuffer{},
        PacketBuffer{}
    };
}

std::vector<PacketBuffer> makeCommonFramePackets() {
    std::vector<PacketBuffer> frames(9);
    for (auto& frame : frames) {
        frame[0] = 0x00;
        for (size_t i = 0; i < 16; ++i) {
            frame[1 + i * 4] = 0x80;
            frame[1 + i * 4 + 1] = 0x00;
            frame[1 + i * 4 + 2] = 0x00;
            frame[1 + i * 4 + 3] = 0x00;
        }
    }
    return frames;
}

PacketBuffer makeEffectPacket(const EffectConfig& config) {
    PacketBuffer buf{};
    buf[0] = 0x00; // Report ID

    // Payload starts at index 1
    buf[1] = static_cast<uint8_t>(config.effect);
    buf[2] = config.r;
    buf[3] = config.g;
    buf[4] = config.b;

    // Byte 8: multicolor flag (index 1 + 8 = 9)
    buf[9] = config.multicolor ? 0x01 : 0x00;

    // Byte 9: Brightness 1-16 (index 1 + 9 = 10)
    buf[10] = static_cast<uint8_t>(std::clamp<int>(config.brightness, 1, 16));

    // Byte 10: Speed 1-16 (index 1 + 10 = 11)
    buf[11] = static_cast<uint8_t>(std::clamp<int>(config.speed, 1, 16));

    // Byte 11: Direction (index 1 + 11 = 12)
    buf[12] = static_cast<uint8_t>(config.direction);

    // Byte 14: 0xAA (index 1 + 14 = 15)
    buf[15] = 0xAA;

    // Byte 15: 0x55 (index 1 + 15 = 16)
    buf[16] = 0x55;

    return buf;
}

PacketBuffer makeFinalizePacket1() {
    PacketBuffer buf{};
    buf[0] = 0x00; // Report ID
    buf[1] = 0x04;
    buf[2] = 0x02;
    return buf;
}

PacketBuffer makeFinalizePacket2() {
    PacketBuffer buf{};
    buf[0] = 0x00; // Report ID
    buf[1] = 0x04;
    buf[2] = 0xF0;
    return buf;
}

DeviceManager::DeviceManager() {
    hid_init();
}

DeviceManager::~DeviceManager() {
    closeDevice();
    hid_exit();
}

std::vector<DeviceInfo> DeviceManager::enumerateDevices() const {
    std::vector<DeviceInfo> devices;
    auto* devList = hid_enumerate(VENDOR_ID, 0);

    for (auto* cur = devList; cur != nullptr; cur = cur->next) {
        if (cur->vendor_id == VENDOR_ID &&
            (cur->product_id == PID_VARIANT_A || cur->product_id == PID_VARIANT_B)) {
            DeviceInfo info;
            info.path = cur->path ? cur->path : "";
            info.vendorId = cur->vendor_id;
            info.productId = cur->product_id;
            info.interfaceNumber = cur->interface_number;
            if (cur->product_string) {
                // Convert wchar_t to string
                std::wstring ws(cur->product_string);
                info.productName = std::string(ws.begin(), ws.end());
            } else {
                info.productName = (cur->product_id == PID_VARIANT_A)
                                       ? "Genesis Thor 404 TKL (0x501B)"
                                       : "Genesis Thor 404 TKL (0x501C)";
            }
            if (cur->serial_number) {
                std::wstring ws(cur->serial_number);
                info.serialNumber = std::string(ws.begin(), ws.end());
            }
            devices.push_back(std::move(info));
        }
    }

    hid_free_enumeration(devList);
    return devices;
}

bool DeviceManager::isConnected() const {
    return m_handle != nullptr;
}

bool DeviceManager::openDevice(uint16_t targetPid) {
    closeDevice();

    auto devices = enumerateDevices();
    if (devices.empty()) {
        return false;
    }

    // Sort/prioritize interface 0 (hidraw node with 64-byte feature report collection)
    std::vector<const DeviceInfo*> candidates;
    for (const auto& dev : devices) {
        if (targetPid == 0 || dev.productId == targetPid) {
            candidates.push_back(&dev);
        }
    }

    if (candidates.empty()) {
        for (const auto& dev : devices) {
            candidates.push_back(&dev);
        }
    }

    // Genesis Thor 404 exposes lighting feature report on Interface 0 (e.g. /dev/hidraw5)
    std::stable_sort(candidates.begin(), candidates.end(), [](const DeviceInfo* a, const DeviceInfo* b) {
        int scoreA = (a->interfaceNumber == 0) ? 0 : 1;
        int scoreB = (b->interfaceNumber == 0) ? 0 : 1;
        return scoreA < scoreB;
    });

    for (const auto* cand : candidates) {
        if (!cand->path.empty()) {
            if (m_debug) {
                std::cout << "[DEBUG] Attempting hid_open_path: " << cand->path
                          << " (Interface " << cand->interfaceNumber << ")\n";
            }
            m_handle = hid_open_path(cand->path.c_str());
            if (m_handle) {
                m_currentInfo = *cand;
                if (m_debug) {
                    std::cout << "[DEBUG] Successfully opened: " << cand->path << "\n";
                }
                return true;
            }
        }
    }

    // Fallback: try open by VID/PID
    uint16_t pidToOpen = targetPid != 0 ? targetPid : candidates.front()->productId;
    if (m_debug) {
        std::cout << "[DEBUG] Fallback hid_open VID: 0x" << std::hex << VENDOR_ID
                  << " PID: 0x" << pidToOpen << std::dec << "\n";
    }
    m_handle = hid_open(VENDOR_ID, pidToOpen, nullptr);
    if (m_handle) {
        m_currentInfo = *candidates.front();
        return true;
    }

    return false;
}

void DeviceManager::closeDevice() {
    if (m_handle) {
        hid_close(m_handle);
        m_handle = nullptr;
    }
    m_currentInfo.reset();
}

bool DeviceManager::sendFeatureReport(const PacketBuffer& buffer) {
    if (!m_handle) return false;
    if (m_debug) {
        std::cout << "[DEBUG] Sending 65-byte feature report:\n  ";
        for (size_t i = 0; i < buffer.size(); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)buffer[i] << " ";
            if ((i + 1) % 16 == 0 && (i + 1) < buffer.size()) {
                std::cout << "\n  ";
            }
        }
        std::cout << std::dec << "\n";
    }
    int res = hid_send_feature_report(m_handle, buffer.data(), buffer.size());
    if (m_debug) {
        std::cout << "[DEBUG] hid_send_feature_report result: " << res << "\n";
    }
    return res >= 0;
}

bool DeviceManager::applyEffect(const EffectConfig& config, std::string* errorMessage) {
    if (!m_handle) {
        if (errorMessage) *errorMessage = "Device not connected";
        return false;
    }

    constexpr auto SLEEP_DURATION = std::chrono::milliseconds(10);

    std::vector<PacketBuffer> sequence;
    sequence.push_back(makeHandshakePacket1());
    sequence.push_back(makeHandshakePacket2());
    sequence.push_back(makeHandshakePacket3());

    auto prefixes = makeCommonPrefixPackets();
    sequence.insert(sequence.end(), prefixes.begin(), prefixes.end());

    auto frames = makeCommonFramePackets();
    sequence.insert(sequence.end(), frames.begin(), frames.end());

    sequence.push_back(makeEffectPacket(config));
    sequence.push_back(makeFinalizePacket1());
    sequence.push_back(makeFinalizePacket2());

    for (size_t i = 0; i < sequence.size(); ++i) {
        if (!sendFeatureReport(sequence[i])) {
            if (errorMessage) {
                const wchar_t* hidErr = hid_error(m_handle);
                std::wstring ws = hidErr ? hidErr : L"Failed to send feature report";
                *errorMessage = std::string(ws.begin(), ws.end());
            }
            return false;
        }

        // Optional drain of response report like official protocol
        PacketBuffer resp{};
        resp[0] = 0x00;
        hid_get_feature_report(m_handle, resp.data(), resp.size());

        if (i + 1 < sequence.size()) {
            std::this_thread::sleep_for(SLEEP_DURATION);
        }
    }

    return true;
}

} // namespace thor
