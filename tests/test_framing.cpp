#include <iostream>
#include <cassert>
#include "devicemanager.h"

int main() {
    using namespace thor;

    // 1. Handshake Packet 1 framing (65 bytes: 0x00, 0x04, 0x02, 62 zeros)
    auto hs1 = makeHandshakePacket1();
    assert(hs1.size() == 65);
    assert(hs1[0] == 0x00);
    assert(hs1[1] == 0x04);
    assert(hs1[2] == 0x02);
    for (size_t i = 3; i < 65; ++i) {
        assert(hs1[i] == 0x00);
    }

    // 2. Handshake Packet 2 framing (65 bytes: 0x00, 0x04, 0x19, 62 zeros)
    auto hs2 = makeHandshakePacket2();
    assert(hs2.size() == 65);
    assert(hs2[0] == 0x00);
    assert(hs2[1] == 0x04);
    assert(hs2[2] == 0x19);
    for (size_t i = 3; i < 65; ++i) {
        assert(hs2[i] == 0x00);
    }

    // 3. Handshake Packet 3 framing (65 bytes: 0x00, 0x04, 0x13, 00*6, 0x12, 55 zeros)
    auto hs3 = makeHandshakePacket3();
    assert(hs3.size() == 65);
    assert(hs3[0] == 0x00);
    assert(hs3[1] == 0x04);
    assert(hs3[2] == 0x13);
    for (size_t i = 3; i < 9; ++i) {
        assert(hs3[i] == 0x00);
    }
    assert(hs3[9] == 0x12);
    for (size_t i = 10; i < 65; ++i) {
        assert(hs3[i] == 0x00);
    }

    // 4. Effect Packet framing
    EffectConfig cfg;
    cfg.effect = EffectId::Wave;
    cfg.r = 0xAA;
    cfg.g = 0xBB;
    cfg.b = 0xCC;
    cfg.multicolor = true;
    cfg.brightness = 14;
    cfg.speed = 8;
    cfg.direction = Direction::Up;

    auto eff = makeEffectPacket(cfg);
    assert(eff.size() == 65);
    assert(eff[0] == 0x00); // Report ID
    assert(eff[1] == 0x12); // Byte 0: Wave Effect ID (0x12)
    assert(eff[2] == 0xAA); // Byte 1: Red
    assert(eff[3] == 0xBB); // Byte 2: Green
    assert(eff[4] == 0xCC); // Byte 3: Blue
    assert(eff[5] == 0x00); // Byte 4
    assert(eff[6] == 0x00); // Byte 5
    assert(eff[7] == 0x00); // Byte 6
    assert(eff[8] == 0x00); // Byte 7
    assert(eff[9] == 0x01); // Byte 8: Multicolor flag (0x01)
    assert(eff[10] == 14);  // Byte 9: Brightness (14)
    assert(eff[11] == 8);   // Byte 10: Speed (8)
    assert(eff[12] == 2);   // Byte 11: Direction Up (2)
    assert(eff[13] == 0x00);
    assert(eff[14] == 0x00);
    assert(eff[15] == 0xAA); // Byte 14: 0xAA
    assert(eff[16] == 0x55); // Byte 15: 0x55
    for (size_t i = 17; i < 65; ++i) {
        assert(eff[i] == 0x00);
    }

    // Test clamped brightness & speed
    cfg.brightness = 99;
    cfg.speed = 0;
    auto effClamped = makeEffectPacket(cfg);
    assert(effClamped[10] == 16);
    assert(effClamped[11] == 1);

    // 5. Finalize Packets
    auto fin1 = makeFinalizePacket1();
    assert(fin1.size() == 65);
    assert(fin1[0] == 0x00);
    assert(fin1[1] == 0x04);
    assert(fin1[2] == 0x02);
    for (size_t i = 3; i < 65; ++i) {
        assert(fin1[i] == 0x00);
    }

    auto fin2 = makeFinalizePacket2();
    assert(fin2.size() == 65);
    assert(fin2[0] == 0x00);
    assert(fin2[1] == 0x04);
    assert(fin2[2] == 0xF0);
    for (size_t i = 3; i < 65; ++i) {
        assert(fin2[i] == 0x00);
    }

    std::cout << "[PASS] All packet framing unit tests passed." << std::endl;
    return 0;
}
