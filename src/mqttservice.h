#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QColor>
#include <QTimer>
#include <functional>
#include "devicemanager.h"

namespace thor {

class MqttLightService : public QObject {
    Q_OBJECT
public:
    explicit MqttLightService(QObject *parent = nullptr);
    ~MqttLightService() override;

    void start(const QString &host = "127.0.0.1", quint16 port = 1883,
               const QString &username = "", const QString &password = "");
    void stop();

    void setApplyCallback(std::function<void(const EffectConfig&)> cb) {
        m_applyCallback = cb;
    }

    void publishState(bool power, uint8_t brightness, const QColor &color, const QString &effect);

signals:
    void connectedChanged(bool connected, const QString &message);

private slots:
    void onConnected();
    void onReadyRead();
    void onError();
    void onConnectTimeout();

private:
    void sendConnect();
    void sendSubscribe(const QString &topic);
    void publish(const QString &topic, const QByteArray &payload, bool retain = false);
    void publishDiscovery();
    void handleIncomingPublish(const QString &topic, const QByteArray &payload);

    QTcpSocket *m_socket{nullptr};
    QString m_host{"127.0.0.1"};
    quint16 m_port{1883};
    QString m_user;
    QString m_pass;
    QTimer *m_reconnectTimer{nullptr};
    QTimer *m_connectTimeoutTimer{nullptr};
    std::function<void(const EffectConfig&)> m_applyCallback;

    bool m_currentPower{true};
    uint8_t m_currentBrightness{16};
    QColor m_currentColor{255, 0, 0};
    QString m_currentEffect{"static"};
};

} // namespace thor
