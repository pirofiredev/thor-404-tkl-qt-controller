#include "mqttservice.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <iostream>

namespace thor {

// Minimal MQTT 3.1.1 implementation over QTcpSocket (zero external dependencies)
MqttLightService::MqttLightService(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)), m_reconnectTimer(new QTimer(this)) {
    connect(m_socket, &QTcpSocket::connected, this, &MqttLightService::onConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MqttLightService::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MqttLightService::onError);

    m_reconnectTimer->setInterval(10000);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (m_socket->state() == QAbstractSocket::UnconnectedState) {
            m_socket->connectToHost(m_host, m_port);
        }
    });
}

MqttLightService::~MqttLightService() {
    stop();
}

void MqttLightService::start(const QString &host, quint16 port, const QString &user, const QString &pass) {
    m_host = host;
    m_port = port;
    m_user = user;
    m_pass = pass;
    m_reconnectTimer->start();
    m_socket->connectToHost(m_host, m_port);
}

void MqttLightService::stop() {
    m_reconnectTimer->stop();
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        // DISCONNECT packet: 0xE0 0x00
        const char disc[] = { (char)0xE0, 0x00 };
        m_socket->write(disc, 2);
        m_socket->disconnectFromHost();
    }
}

static QByteArray encodeString(const QString &str) {
    QByteArray utf = str.toUtf8();
    QByteArray out;
    out.append((char)((utf.size() >> 8) & 0xFF));
    out.append((char)(utf.size() & 0xFF));
    out.append(utf);
    return out;
}

static QByteArray encodeLength(int len) {
    QByteArray out;
    do {
        char byte = len % 128;
        len /= 128;
        if (len > 0) byte |= 0x80;
        out.append(byte);
    } while (len > 0);
    return out;
}

void MqttLightService::sendConnect() {
    QByteArray varHeader;
    varHeader.append(encodeString("MQTT")); // Protocol name
    varHeader.append((char)0x04);           // Level 4 (v3.1.1)

    char flags = 0x02; // Clean session
    if (!m_user.isEmpty()) flags |= 0x80;
    if (!m_pass.isEmpty()) flags |= 0x40;
    varHeader.append(flags);
    varHeader.append((char)0x00); // Keep alive 60s
    varHeader.append((char)0x3C);

    QByteArray payload;
    payload.append(encodeString("thor_404_controller_" + QString::number(QCoreApplication::applicationPid())));
    if (!m_user.isEmpty()) payload.append(encodeString(m_user));
    if (!m_pass.isEmpty()) payload.append(encodeString(m_pass));

    QByteArray packet;
    packet.append((char)0x10); // CONNECT
    packet.append(encodeLength(varHeader.size() + payload.size()));
    packet.append(varHeader);
    packet.append(payload);

    m_socket->write(packet);
}

void MqttLightService::onConnected() {
    sendConnect();
}

void MqttLightService::sendSubscribe(const QString &topic) {
    QByteArray body;
    // Packet ID 1
    body.append((char)0x00);
    body.append((char)0x01);
    body.append(encodeString(topic));
    body.append((char)0x00); // QoS 0

    QByteArray packet;
    packet.append((char)0x82); // SUBSCRIBE QoS 1
    packet.append(encodeLength(body.size()));
    packet.append(body);

    m_socket->write(packet);
}

void MqttLightService::publish(const QString &topic, const QByteArray &payload, bool retain) {
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray varHeader = encodeString(topic);
    QByteArray packet;
    char headerByte = 0x30 | (retain ? 0x01 : 0x00); // PUBLISH QoS 0
    packet.append(headerByte);
    packet.append(encodeLength(varHeader.size() + payload.size()));
    packet.append(varHeader);
    packet.append(payload);

    m_socket->write(packet);
}

void MqttLightService::publishDiscovery() {
    // Home Assistant MQTT Discovery for light entity
    QJsonObject device;
    device["identifiers"] = QJsonArray{"thor_404_tkl_331a"};
    device["name"] = "Genesis Thor 404 TKL";
    device["manufacturer"] = "Genesis";
    device["model"] = "Thor 404 TKL";

    QJsonObject config;
    config["name"] = "Genesis Thor 404 Keyboard RGB";
    config["unique_id"] = "thor_404_rgb_light";
    config["schema"] = "json";
    config["command_topic"] = "homeassistant/light/thor_404/set";
    config["state_topic"] = "homeassistant/light/thor_404/state";
    config["brightness"] = true;
    config["brightness_scale"] = 16;
    config["supported_color_modes"] = QJsonArray{"rgb"};
    config["effect"] = true;
    config["effect_list"] = QJsonArray{
        "static", "breathing", "neon", "wave", "colourful",
        "rainbow", "spectrum", "prismo", "circle", "stars",
        "raindrops", "sinewave", "shuttle", "shockwave", "explosion", "off"
    };
    config["device"] = device;

    QJsonDocument doc(config);
    publish("homeassistant/light/thor_404/config", doc.toJson(QJsonDocument::Compact), true);

    // Subscribe to commands
    sendSubscribe("homeassistant/light/thor_404/set");

    // Publish initial state
    publishState(m_currentPower, m_currentBrightness, m_currentColor, m_currentEffect);
}

void MqttLightService::publishState(bool power, uint8_t brightness, const QColor &color, const QString &effect) {
    m_currentPower = power;
    m_currentBrightness = brightness;
    m_currentColor = color;
    m_currentEffect = effect;

    QJsonObject state;
    state["state"] = power ? "ON" : "OFF";
    state["brightness"] = brightness;
    QJsonObject rgb;
    rgb["r"] = color.red();
    rgb["g"] = color.green();
    rgb["b"] = color.blue();
    state["color"] = rgb;
    state["effect"] = effect;

    QJsonDocument doc(state);
    publish("homeassistant/light/thor_404/state", doc.toJson(QJsonDocument::Compact), true);
}

void MqttLightService::onReadyRead() {
    QByteArray data = m_socket->readAll();
    int i = 0;
    while (i < data.size()) {
        uint8_t header = static_cast<uint8_t>(data[i++]);
        uint8_t type = header >> 4;

        // Decode remaining length
        int remLen = 0;
        int multiplier = 1;
        while (i < data.size()) {
            uint8_t encodedByte = static_cast<uint8_t>(data[i++]);
            remLen += (encodedByte & 0x7F) * multiplier;
            multiplier *= 128;
            if ((encodedByte & 0x80) == 0) break;
        }

        if (type == 0x02) { // CONNACK
            if (i + 1 < data.size()) {
                uint8_t returnCode = static_cast<uint8_t>(data[i + 1]);
                if (returnCode == 0) {
                    publishDiscovery();
                }
            }
            i += remLen;
        } else if (type == 0x03) { // PUBLISH
            int start = i;
            if (start + 2 <= data.size()) {
                int topicLen = (static_cast<uint8_t>(data[start]) << 8) | static_cast<uint8_t>(data[start + 1]);
                QString topic = QString::fromUtf8(data.mid(start + 2, topicLen));
                int payloadOffset = start + 2 + topicLen;
                int payloadLen = remLen - (2 + topicLen);
                QByteArray payload = data.mid(payloadOffset, payloadLen);
                handleIncomingPublish(topic, payload);
            }
            i += remLen;
        } else {
            i += remLen;
        }
    }
}

void MqttLightService::handleIncomingPublish(const QString &topic, const QByteArray &payload) {
    if (topic != "homeassistant/light/thor_404/set") return;

    QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) return;
    QJsonObject obj = doc.object();

    EffectConfig cfg;

    if (obj.contains("state")) {
        QString s = obj["state"].toString().toUpper();
        m_currentPower = (s == "ON");
        if (!m_currentPower) {
            cfg.effect = EffectId::Off;
        }
    }

    if (obj.contains("brightness")) {
        m_currentBrightness = static_cast<uint8_t>(std::clamp(obj["brightness"].toInt(), 1, 16));
    }
    cfg.brightness = m_currentBrightness;

    if (obj.contains("color")) {
        QJsonObject colorObj = obj["color"].toObject();
        m_currentColor = QColor(colorObj["r"].toInt(255), colorObj["g"].toInt(0), colorObj["b"].toInt(0));
    }
    cfg.r = m_currentColor.red();
    cfg.g = m_currentColor.green();
    cfg.b = m_currentColor.blue();

    if (obj.contains("effect")) {
        m_currentEffect = obj["effect"].toString().toLower();
        if (m_currentEffect == "static") cfg.effect = EffectId::Static;
        else if (m_currentEffect == "breathing") cfg.effect = EffectId::Breathing;
        else if (m_currentEffect == "neon") cfg.effect = EffectId::Neon;
        else if (m_currentEffect == "wave") cfg.effect = EffectId::Wave;
        else if (m_currentEffect == "colourful") cfg.effect = EffectId::Colourful;
        else if (m_currentEffect == "rainbow") cfg.effect = EffectId::Rainbow;
        else if (m_currentEffect == "spectrum") cfg.effect = EffectId::Spectrum;
        else if (m_currentEffect == "prismo") cfg.effect = EffectId::Prismo;
        else if (m_currentEffect == "circle") cfg.effect = EffectId::Circle;
        else if (m_currentEffect == "stars") cfg.effect = EffectId::Stars;
        else if (m_currentEffect == "raindrops") cfg.effect = EffectId::Raindrops;
        else if (m_currentEffect == "sinewave") cfg.effect = EffectId::SineWave;
        else if (m_currentEffect == "shuttle") cfg.effect = EffectId::Shuttle;
        else if (m_currentEffect == "shockwave") cfg.effect = EffectId::Shockwave;
        else if (m_currentEffect == "explosion") cfg.effect = EffectId::Explosion;
        else if (m_currentEffect == "off") {
            cfg.effect = EffectId::Off;
            m_currentPower = false;
        }
    } else if (m_currentPower) {
        cfg.effect = EffectId::Static;
    }

    if (m_applyCallback) {
        m_applyCallback(cfg);
    }

    publishState(m_currentPower, m_currentBrightness, m_currentColor, m_currentEffect);
}

void MqttLightService::onError() {
    // Reconnect handled by timer
}

} // namespace thor
