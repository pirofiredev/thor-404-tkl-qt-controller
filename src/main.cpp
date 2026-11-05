#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <iostream>
#include <cassert>
#include "mainwindow.h"
#include "devicemanager.h"

static void runPacketFramingTests() {
    using namespace thor;

    // 1. Handshake Packet 1 framing
    auto hs1 = makeHandshakePacket1();
    assert(hs1.size() == 65);
    assert(hs1[0] == 0x00);
    assert(hs1[1] == 0x04);
    assert(hs1[2] == 0x02);
    for (size_t i = 3; i < 65; ++i) {
        assert(hs1[i] == 0x00);
    }

    // 2. Handshake Packet 2 framing
    auto hs2 = makeHandshakePacket2();
    assert(hs2.size() == 65);
    assert(hs2[0] == 0x00);
    assert(hs2[1] == 0x04);
    assert(hs2[2] == 0x19);
    for (size_t i = 3; i < 65; ++i) {
        assert(hs2[i] == 0x00);
    }

    // 3. Handshake Packet 3 framing
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
    cfg.r = 0x12;
    cfg.g = 0x34;
    cfg.b = 0x56;
    cfg.multicolor = true;
    cfg.brightness = 10;
    cfg.speed = 12;
    cfg.direction = Direction::Down;

    auto eff = makeEffectPacket(cfg);
    assert(eff.size() == 65);
    assert(eff[0] == 0x00); // Report ID
    assert(eff[1] == 0x12); // Byte 0: Wave Effect ID
    assert(eff[2] == 0x12); // Byte 1: R
    assert(eff[3] == 0x34); // Byte 2: G
    assert(eff[4] == 0x56); // Byte 3: B
    assert(eff[5] == 0x00); // Byte 4
    assert(eff[6] == 0x00); // Byte 5
    assert(eff[7] == 0x00); // Byte 6
    assert(eff[8] == 0x00); // Byte 7
    assert(eff[9] == 0x01); // Byte 8: Multicolor
    assert(eff[10] == 10);  // Byte 9: Brightness
    assert(eff[11] == 12);  // Byte 10: Speed
    assert(eff[12] == 3);   // Byte 11: Direction Down (3)
    assert(eff[13] == 0x00);
    assert(eff[14] == 0x00);
    assert(eff[15] == 0xAA); // Byte 14: 0xAA
    assert(eff[16] == 0x55); // Byte 15: 0x55
    for (size_t i = 17; i < 65; ++i) {
        assert(eff[i] == 0x00);
    }

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

    std::cout << "[PASS] All packet framing assertions passed." << std::endl;
}

static void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --help, -h               Show this help message\n"
              << "  --debug, -d              Enable verbose debug output\n"
              << "  --test-framing           Run packet buffer framing self-tests and exit\n"
              << "  --list-devices           List detected Genesis devices and exit\n"
              << "  --set-static <R> <G> <B> Apply static color (0-255) and exit without GUI\n"
              << "  --set-effect <name>      Apply effect (static|breathing|neon|wave|off) without GUI\n"
              << "  --brightness <1-16>      Set brightness (default: 16)\n"
              << "  --speed <1-16>           Set speed (default: 16)\n"
              << "  --direction <0-3>        Direction: 0=Right, 1=Left, 2=Up, 3=Down\n"
              << "  --multicolor             Enable rainbow/multicolor mode\n";
}

int main(int argc, char *argv[]) {
    using namespace thor;

    bool debug = false;
    bool cliMode = false;
    bool listDevs = false;
    EffectConfig cfg;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--test-framing") {
            runPacketFramingTests();
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--debug" || arg == "-d") {
            debug = true;
        } else if (arg == "--list-devices") {
            listDevs = true;
            cliMode = true;
        } else if (arg == "--profile" && i + 1 < argc) {
            cliMode = true;
            int prof = std::stoi(argv[++i]);
            DeviceManager dm;
            dm.setDebug(debug);
            if (!dm.openDevice(0)) {
                std::cerr << "Failed to open device.\n";
                return 1;
            }
            std::string err;
            if (!dm.setProfile(static_cast<uint8_t>(prof), &err)) {
                std::cerr << "Profile switch failed: " << err << "\n";
                return 1;
            }
            std::cout << "Profile switched to " << prof << "\n";
            return 0;
        } else if (arg == "--set-static" && i + 3 < argc) {
            cliMode = true;
            cfg.effect = EffectId::Static;
            cfg.r = static_cast<uint8_t>(std::clamp(std::stoi(argv[++i]), 0, 255));
            cfg.g = static_cast<uint8_t>(std::clamp(std::stoi(argv[++i]), 0, 255));
            cfg.b = static_cast<uint8_t>(std::clamp(std::stoi(argv[++i]), 0, 255));
        } else if (arg == "--set-effect" && i + 1 < argc) {
            cliMode = true;
            std::string eff = argv[++i];
            if (eff == "static") cfg.effect = EffectId::Static;
            else if (eff == "breathing") cfg.effect = EffectId::Breathing;
            else if (eff == "neon") cfg.effect = EffectId::Neon;
            else if (eff == "wave") cfg.effect = EffectId::Wave;
            else if (eff == "colourful") cfg.effect = EffectId::Colourful;
            else if (eff == "rainbow") cfg.effect = EffectId::Rainbow;
            else if (eff == "spectrum") cfg.effect = EffectId::Spectrum;
            else if (eff == "prismo") cfg.effect = EffectId::Prismo;
            else if (eff == "circle") cfg.effect = EffectId::Circle;
            else if (eff == "stars") cfg.effect = EffectId::Stars;
            else if (eff == "raindrops") cfg.effect = EffectId::Raindrops;
            else if (eff == "sinewave") cfg.effect = EffectId::SineWave;
            else if (eff == "shuttle") cfg.effect = EffectId::Shuttle;
            else if (eff == "shockwave") cfg.effect = EffectId::Shockwave;
            else if (eff == "explosion") cfg.effect = EffectId::Explosion;
            else if (eff == "off") cfg.effect = EffectId::Off;
            else {
                std::cerr << "Unknown effect: " << eff << "\n";
                return 1;
            }
        } else if (arg == "--brightness" && i + 1 < argc) {
            cfg.brightness = static_cast<uint8_t>(std::clamp(std::stoi(argv[++i]), 1, 16));
        } else if (arg == "--speed" && i + 1 < argc) {
            cfg.speed = static_cast<uint8_t>(std::clamp(std::stoi(argv[++i]), 1, 16));
        } else if (arg == "--direction" && i + 1 < argc) {
            cfg.direction = static_cast<Direction>(std::clamp(std::stoi(argv[++i]), 0, 3));
        } else if (arg == "--multicolor") {
            cfg.multicolor = true;
        }
    }

    if (cliMode) {
        DeviceManager dm;
        dm.setDebug(debug);

        if (listDevs) {
            auto devs = dm.enumerateDevices();
            std::cout << "Detected " << devs.size() << " device interface(s):\n";
            for (const auto& d : devs) {
                std::cout << "  - " << d.productName
                          << " (VID: 0x" << std::hex << d.vendorId
                          << " PID: 0x" << d.productId << std::dec
                          << " Interface: " << d.interfaceNumber
                          << " Path: " << d.path << ")\n";
            }
            return 0;
        }

        if (!dm.openDevice(0)) {
            std::cerr << "Error: Failed to open Genesis keyboard device. Check permissions or udev rules.\n";
            return 1;
        }

        std::string err;
        if (!dm.applyEffect(cfg, &err)) {
            std::cerr << "Error applying effect: " << err << "\n";
            return 1;
        }

        std::cout << "Settings applied successfully!\n";
        return 0;
    }

    // ponytail: run framing self-check on startup to ensure zero protocol drift
    runPacketFramingTests();

    QApplication app(argc, argv);
    app.setApplicationName("thor-404-controller");
    app.setApplicationDisplayName("Genesis Thor 404 TKL RGB Controller");
    app.setWindowIcon(QIcon(":/resources/genesis-logo.svg"));
    app.setDesktopFileName("thor-404-controller.desktop");

    // Ensure desktop entry and icon exist in user's XDG data directory if running uninstalled from build/
    QString userAppDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(userAppDir);
    QString userDesktopFile = userAppDir + "/thor-404-controller.desktop";
    if (!QFile::exists(userDesktopFile) && !QFile::exists("/usr/share/applications/thor-404-controller.desktop")) {
        QFile src(":/resources/genesis-logo.svg");
        QString userIconDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons/hicolor/scalable/apps";
        QDir().mkpath(userIconDir);
        if (src.open(QIODevice::ReadOnly)) {
            QFile iconOut(userIconDir + "/thor-404-controller.svg");
            if (iconOut.open(QIODevice::WriteOnly)) {
                iconOut.write(src.readAll());
                iconOut.close();
            }
            src.close();
        }

        QFile df(userDesktopFile);
        if (df.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString content =
                "[Desktop Entry]\n"
                "Type=Application\n"
                "Name=Genesis Thor 404 Controller\n"
                "Comment=Native Linux Controller for Genesis Thor 404 TKL Keyboard\n"
                "Exec=" + QCoreApplication::applicationFilePath() + "\n"
                "Icon=thor-404-controller\n"
                "Terminal=false\n"
                "Categories=HardwareSettings;Settings;Utility;Qt;\n"
                "StartupWMClass=thor-404-controller\n";
            df.write(content.toUtf8());
            df.close();
        }
    }

    thor::MainWindow window;
    window.show();

    return app.exec();
}
