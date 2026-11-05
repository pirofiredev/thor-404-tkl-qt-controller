#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QFileSystemWatcher>
#include <functional>
#include <string>

namespace thor {

class SysfsLedService : public QObject {
    Q_OBJECT
public:
    explicit SysfsLedService(QObject *parent = nullptr);
    ~SysfsLedService() override;

    bool start(const std::string &ledName = "genesis_thor404::kbd_backlight");
    void stop();

    void setBrightnessCallback(std::function<void(uint8_t)> cb) {
        m_callback = cb;
    }

    void updateBrightness(uint8_t brightness);

private slots:
    void onFifoReady();

private:
    std::string m_runtimeDir;
    std::string m_fifoPath;
    std::string m_sysfsDir;
    int m_fifoFd{-1};
    QSocketNotifier *m_notifier{nullptr};
    std::function<void(uint8_t)> m_callback;
    uint8_t m_currentBrightness{16};
};

} // namespace thor
