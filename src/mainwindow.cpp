#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QColorDialog>
#include <QTimer>
#include <QPixmap>
#include <QPainter>

namespace thor {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Genesis Thor 404 TKL RGB Controller"));
    resize(480, 520);
    setMinimumWidth(420);

    setupUi();

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onRefreshDevices);
    m_pollTimer->start(2000);

    onRefreshDevices();
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(14);

    // Message / Error label
    m_msgLabel = new QLabel(central);
    m_msgLabel->setAlignment(Qt::AlignCenter);
    m_msgLabel->setWordWrap(true);

    // 1. Device status and model selection
    auto *deviceGroup = new QGroupBox(tr("Device Connection"), central);
    auto *deviceLayout = new QFormLayout(deviceGroup);

    m_statusBadge = new QLabel(deviceGroup);
    m_statusBadge->setFrameShape(QFrame::StyledPanel);
    m_statusBadge->setAlignment(Qt::AlignCenter);

    m_modelCombo = new QComboBox(deviceGroup);
    m_modelCombo->addItem(tr("Auto-detect"), 0);
    m_modelCombo->addItem(tr("Genesis Thor 404 TKL (0x501B)"), static_cast<int>(PID_VARIANT_A));
    m_modelCombo->addItem(tr("Genesis Thor 404 TKL (0x501C)"), static_cast<int>(PID_VARIANT_B));
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModelSelected);

    deviceLayout->addRow(tr("Status:"), m_statusBadge);
    deviceLayout->addRow(tr("Model:"), m_modelCombo);
    mainLayout->addWidget(deviceGroup);

    // 2. Lighting configuration
    auto *lightingGroup = new QGroupBox(tr("Lighting Settings"), central);
    auto *form = new QFormLayout(lightingGroup);

    // Effect
    m_effectCombo = new QComboBox(lightingGroup);
    m_effectCombo->addItem(tr("Static"), static_cast<int>(EffectId::Static));
    m_effectCombo->addItem(tr("Breathing"), static_cast<int>(EffectId::Breathing));
    m_effectCombo->addItem(tr("Neon"), static_cast<int>(EffectId::Neon));
    m_effectCombo->addItem(tr("Wave"), static_cast<int>(EffectId::Wave));
    m_effectCombo->addItem(tr("Colourful"), static_cast<int>(EffectId::Colourful));
    m_effectCombo->addItem(tr("Rainbow"), static_cast<int>(EffectId::Rainbow));
    m_effectCombo->addItem(tr("Spectrum"), static_cast<int>(EffectId::Spectrum));
    m_effectCombo->addItem(tr("Prismo"), static_cast<int>(EffectId::Prismo));
    m_effectCombo->addItem(tr("Circle"), static_cast<int>(EffectId::Circle));
    m_effectCombo->addItem(tr("Stars"), static_cast<int>(EffectId::Stars));
    m_effectCombo->addItem(tr("Raindrops"), static_cast<int>(EffectId::Raindrops));
    m_effectCombo->addItem(tr("Sine Wave"), static_cast<int>(EffectId::SineWave));
    m_effectCombo->addItem(tr("Shuttle"), static_cast<int>(EffectId::Shuttle));
    m_effectCombo->addItem(tr("Shockwave"), static_cast<int>(EffectId::Shockwave));
    m_effectCombo->addItem(tr("Explosion"), static_cast<int>(EffectId::Explosion));
    m_effectCombo->addItem(tr("Escape Beam"), static_cast<int>(EffectId::EscapeBeam));
    m_effectCombo->addItem(tr("Off"), static_cast<int>(EffectId::Off));
    connect(m_effectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEffectChanged);
    form->addRow(tr("Effect:"), m_effectCombo);

    // Color picker button & rainbow toggle
    auto *colorRowLayout = new QHBoxLayout();
    m_colorBtn = new QPushButton(tr("Select Color..."), lightingGroup);
    updateSwatch();
    connect(m_colorBtn, &QPushButton::clicked, this, &MainWindow::onPickColor);

    m_multicolorCheck = new QCheckBox(tr("Rainbow / Multicolor"), lightingGroup);
    colorRowLayout->addWidget(m_colorBtn);
    colorRowLayout->addWidget(m_multicolorCheck);
    form->addRow(tr("Color:"), colorRowLayout);

    // Brightness slider (1 - 16)
    auto *brightLayout = new QHBoxLayout();
    m_brightnessSlider = new QSlider(Qt::Horizontal, lightingGroup);
    m_brightnessSlider->setRange(1, 16);
    m_brightnessSlider->setValue(16);
    m_brightnessValLabel = new QLabel(QString::number(m_brightnessSlider->value()), lightingGroup);
    m_brightnessValLabel->setFixedWidth(24);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, [this](int val) {
        m_brightnessValLabel->setText(QString::number(val));
    });
    brightLayout->addWidget(m_brightnessSlider);
    brightLayout->addWidget(m_brightnessValLabel);
    form->addRow(tr("Brightness:"), brightLayout);

    // Speed slider (1 - 16)
    auto *speedLayout = new QHBoxLayout();
    m_speedSlider = new QSlider(Qt::Horizontal, lightingGroup);
    m_speedSlider->setRange(1, 16);
    m_speedSlider->setValue(16);
    m_speedValLabel = new QLabel(QString::number(m_speedSlider->value()), lightingGroup);
    m_speedValLabel->setFixedWidth(24);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int val) {
        m_speedValLabel->setText(QString::number(val));
    });
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedValLabel);
    form->addRow(tr("Speed:"), speedLayout);

    // Direction dropdown
    m_directionCombo = new QComboBox(lightingGroup);
    m_directionCombo->addItem(tr("Right"), static_cast<int>(Direction::Right));
    m_directionCombo->addItem(tr("Left"), static_cast<int>(Direction::Left));
    m_directionCombo->addItem(tr("Up"), static_cast<int>(Direction::Up));
    m_directionCombo->addItem(tr("Down"), static_cast<int>(Direction::Down));
    form->addRow(tr("Direction:"), m_directionCombo);

    mainLayout->addWidget(lightingGroup);
    mainLayout->addWidget(m_msgLabel);

    // Apply button
    m_applyBtn = new QPushButton(tr("Apply"), central);
    m_applyBtn->setDefault(true);
    m_applyBtn->setFixedHeight(36);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    mainLayout->addWidget(m_applyBtn);

    updateStatusBadge(false);
}

void MainWindow::updateStatusBadge(bool connected) {
    if (!m_statusBadge) return;

    if (connected) {
        auto dev = m_deviceManager.currentDevice();
        QString text = tr("● Connected");
        if (dev.has_value()) {
            text += QString(" (0x%1)").arg(dev->productId, 4, 16, QChar('0')).toUpper();
        }
        m_statusBadge->setText(text);
        m_statusBadge->setStyleSheet("color: #388e3c; font-weight: bold; padding: 4px;");
        if (m_msgLabel) m_msgLabel->clear();
    } else {
        auto devs = m_deviceManager.enumerateDevices();
        if (!devs.empty()) {
            m_statusBadge->setText(tr("○ Detected (Permission Denied)"));
            m_statusBadge->setStyleSheet("color: #f57c00; font-weight: bold; padding: 4px;");
            if (m_msgLabel) {
                m_msgLabel->setText(tr("Device detected on USB but hidraw permissions are missing.<br>Run: <code>sudo cp 99-genesis-thor.rules /etc/udev/rules.d/ && sudo udevadm trigger</code>"));
            }
        } else {
            m_statusBadge->setText(tr("○ Disconnected"));
            m_statusBadge->setStyleSheet("color: #d32f2f; font-weight: bold; padding: 4px;");
            if (m_msgLabel) m_msgLabel->clear();
        }
    }
}

void MainWindow::updateSwatch() {
    QPixmap pix(20, 20);
    pix.fill(m_selectedColor);
    QPainter p(&pix);
    p.setPen(palette().color(QPalette::WindowText));
    p.drawRect(0, 0, 19, 19);
    p.end();

    m_colorBtn->setIcon(QIcon(pix));
    m_colorBtn->setText(m_selectedColor.name().toUpper());
}

void MainWindow::onRefreshDevices() {
    auto devices = m_deviceManager.enumerateDevices();
    bool found = !devices.empty();

    if (found && !m_deviceManager.isConnected()) {
        int targetPid = m_modelCombo->currentData().toInt();
        m_deviceManager.openDevice(targetPid);
    } else if (!found && m_deviceManager.isConnected()) {
        m_deviceManager.closeDevice();
    }

    updateStatusBadge(m_deviceManager.isConnected());
}

void MainWindow::onModelSelected(int) {
    if (m_deviceManager.isConnected()) {
        m_deviceManager.closeDevice();
    }
    int targetPid = m_modelCombo->currentData().toInt();
    m_deviceManager.openDevice(targetPid);
    updateStatusBadge(m_deviceManager.isConnected());
}

void MainWindow::onEffectChanged(int) {
    auto eff = static_cast<EffectId>(m_effectCombo->currentData().toInt());
    bool isOff = (eff == EffectId::Off);
    m_colorBtn->setEnabled(!isOff);
    m_multicolorCheck->setEnabled(!isOff);
    m_brightnessSlider->setEnabled(!isOff);
    m_speedSlider->setEnabled(!isOff && eff != EffectId::Static);
    m_directionCombo->setEnabled(!isOff && (eff == EffectId::Wave));
}

void MainWindow::onPickColor() {
    QColor chosen = QColorDialog::getColor(m_selectedColor, this, tr("Select Lighting Color"));
    if (chosen.isValid()) {
        m_selectedColor = chosen;
        updateSwatch();
    }
}

void MainWindow::onApply() {
    if (!m_deviceManager.isConnected()) {
        // Try to connect once more
        int targetPid = m_modelCombo->currentData().toInt();
        m_deviceManager.openDevice(targetPid);
        updateStatusBadge(m_deviceManager.isConnected());
    }

    if (!m_deviceManager.isConnected()) {
        m_msgLabel->setText(tr("Device not found. Check USB connection and udev rules."));
        return;
    }

    EffectConfig cfg;
    cfg.effect = static_cast<EffectId>(m_effectCombo->currentData().toInt());
    cfg.r = static_cast<uint8_t>(m_selectedColor.red());
    cfg.g = static_cast<uint8_t>(m_selectedColor.green());
    cfg.b = static_cast<uint8_t>(m_selectedColor.blue());
    cfg.multicolor = m_multicolorCheck->isChecked();
    cfg.brightness = static_cast<uint8_t>(m_brightnessSlider->value());
    cfg.speed = static_cast<uint8_t>(m_speedSlider->value());
    cfg.direction = static_cast<Direction>(m_directionCombo->currentData().toInt());

    std::string err;
    if (m_deviceManager.applyEffect(cfg, &err)) {
        m_msgLabel->setText(tr("Settings applied successfully!"));
    } else {
        m_msgLabel->setText(tr("Failed to apply settings: %1").arg(QString::fromStdString(err)));
    }
}

} // namespace thor
