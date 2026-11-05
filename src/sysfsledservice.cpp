#include "sysfsledservice.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace thor {

SysfsLedService::SysfsLedService(QObject *parent)
    : QObject(parent) {
    const char *xdg = std::getenv("XDG_RUNTIME_DIR");
    m_runtimeDir = xdg ? xdg : "/tmp";
}

SysfsLedService::~SysfsLedService() {
    stop();
}

bool SysfsLedService::start(const std::string &ledName) {
    stop();

    m_fifoPath = m_runtimeDir + "/" + ledName + ".brightness";
    unlink(m_fifoPath.c_str());
    if (mkfifo(m_fifoPath.c_str(), 0666) != 0) {
        return false;
    }
    chmod(m_fifoPath.c_str(), 0666);

    // Open O_RDWR so it won't trigger EOF when writers close
    m_fifoFd = open(m_fifoPath.c_str(), O_RDWR | O_NONBLOCK);
    if (m_fifoFd < 0) {
        return false;
    }

    m_notifier = new QSocketNotifier(m_fifoFd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &SysfsLedService::onFifoReady);

    // Expose standard max_brightness file alongside fifo
    std::string maxFile = m_runtimeDir + "/" + ledName + ".max_brightness";
    std::ofstream maxOut(maxFile);
    if (maxOut) {
        maxOut << "16\n";
    }

    return true;
}

void SysfsLedService::stop() {
    if (m_notifier) {
        delete m_notifier;
        m_notifier = nullptr;
    }
    if (m_fifoFd >= 0) {
        close(m_fifoFd);
        m_fifoFd = -1;
    }
    if (!m_fifoPath.empty()) {
        unlink(m_fifoPath.c_str());
    }
}

void SysfsLedService::onFifoReady() {
    char buf[64];
    ssize_t n = read(m_fifoFd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        try {
            int val = std::stoi(buf);
            val = std::clamp(val, 1, 16);
            m_currentBrightness = static_cast<uint8_t>(val);
            if (m_callback) {
                m_callback(m_currentBrightness);
            }
        } catch (...) {}
    }
}

void SysfsLedService::updateBrightness(uint8_t brightness) {
    m_currentBrightness = std::clamp<uint8_t>(brightness, 1, 16);
}

} // namespace thor
