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
#include <QMenu>
#include <QAction>
#include <QTabWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QEvent>

namespace thor {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Genesis Thor 404 TKL"));
    resize(520, 590);
    setMinimumWidth(460);

    // Initialize default quick action slots
    m_quickSlots[0] = { tr("Red Static"), EffectId::Static, QColor(255, 0, 0), false };
    m_quickSlots[1] = { tr("Wave Rainbow"), EffectId::Wave, QColor(255, 0, 0), true };

    setupUi();
    setupTray();

    // Connect Home Assistant MQTT apply callback
    m_mqttService.setApplyCallback([this](const EffectConfig &cfg) {
        applyConfig(cfg);
    });

    // Connect KDE virtual sysfs LED brightness callback
    m_sysfsLedService.setBrightnessCallback([this](uint8_t brightness) {
        if (m_brightnessSlider) {
            m_brightnessSlider->setValue(brightness);
        }
        EffectConfig cfg;
        cfg.effect = static_cast<EffectId>(m_effectCombo->currentData().toInt());
        cfg.r = static_cast<uint8_t>(m_selectedColor.red());
        cfg.g = static_cast<uint8_t>(m_selectedColor.green());
        cfg.b = static_cast<uint8_t>(m_selectedColor.blue());
        cfg.multicolor = m_multicolorCheck->isChecked();
        cfg.brightness = brightness;
        cfg.speed = static_cast<uint8_t>(m_speedSlider->value());
        cfg.direction = static_cast<Direction>(m_directionCombo->currentData().toInt());
        applyConfig(cfg);
    });

    m_sysfsLedService.start("genesis_thor404::kbd_backlight");

    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &MainWindow::onRefreshDevices);
    m_pollTimer->start(2000);

    onRefreshDevices();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Minimize to tray on close to maintain background services (KDE applet, MQTT)
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        updateTrayIcon();
        updateSwatch();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(14);

    // Header: Status Card (JamesDSP style clean top-bar)
    auto *topCard = new QGroupBox(this);
    auto *topLayout = new QHBoxLayout(topCard);
    topLayout->setContentsMargins(12, 10, 12, 10);

    m_statusBadge = new QLabel(topCard);
    m_statusBadge->setFrameShape(QFrame::NoFrame);
    m_statusBadge->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_modelCombo = new QComboBox(topCard);
    m_modelCombo->addItem(tr("Auto-detect"), 0);
    m_modelCombo->addItem(tr("Genesis Thor 404 TKL (0x501B)"), static_cast<int>(PID_VARIANT_A));
    m_modelCombo->addItem(tr("Genesis Thor 404 TKL (0x501C)"), static_cast<int>(PID_VARIANT_B));
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModelSelected);

    topLayout->addWidget(m_statusBadge, 1);
    topLayout->addWidget(new QLabel(tr("Model:"), topCard));
    topLayout->addWidget(m_modelCombo);
    mainLayout->addWidget(topCard);

    // Tab Widget (JamesDSP card tabs)
    m_tabs = new QTabWidget(central);

    // TAB 1: Lighting
    auto *lightingTab = new QWidget();
    auto *lightingLayout = new QVBoxLayout(lightingTab);
    lightingLayout->setContentsMargins(12, 12, 12, 12);
    lightingLayout->setSpacing(12);

    auto *form = new QFormLayout();
    form->setSpacing(10);

    // Effect
    m_effectCombo = new QComboBox(lightingTab);
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
    m_effectCombo->addItem(tr("Off"), static_cast<int>(EffectId::Off));
    connect(m_effectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEffectChanged);
    form->addRow(tr("Effect:"), m_effectCombo);

    // Color picker
    auto *colorRowLayout = new QHBoxLayout();
    m_colorBtn = new QPushButton(tr("Select Color..."), lightingTab);
    updateSwatch();
    connect(m_colorBtn, &QPushButton::clicked, this, &MainWindow::onPickColor);

    m_multicolorCheck = new QCheckBox(tr("Multicolor / Rainbow"), lightingTab);
    colorRowLayout->addWidget(m_colorBtn);
    colorRowLayout->addWidget(m_multicolorCheck);
    form->addRow(tr("Color:"), colorRowLayout);

    // Brightness slider (1 - 16)
    auto *brightLayout = new QHBoxLayout();
    m_brightnessSlider = new QSlider(Qt::Horizontal, lightingTab);
    m_brightnessSlider->setRange(1, 16);
    m_brightnessSlider->setValue(16);
    m_brightnessValLabel = new QLabel(QString::number(m_brightnessSlider->value()), lightingTab);
    m_brightnessValLabel->setFixedWidth(24);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, [this](int val) {
        m_brightnessValLabel->setText(QString::number(val));
    });
    brightLayout->addWidget(m_brightnessSlider);
    brightLayout->addWidget(m_brightnessValLabel);
    form->addRow(tr("Brightness:"), brightLayout);

    // Speed slider (1 - 16)
    auto *speedLayout = new QHBoxLayout();
    m_speedSlider = new QSlider(Qt::Horizontal, lightingTab);
    m_speedSlider->setRange(1, 16);
    m_speedSlider->setValue(16);
    m_speedValLabel = new QLabel(QString::number(m_speedSlider->value()), lightingTab);
    m_speedValLabel->setFixedWidth(24);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int val) {
        m_speedValLabel->setText(QString::number(val));
    });
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedValLabel);
    form->addRow(tr("Speed:"), speedLayout);

    // Direction dropdown
    m_directionCombo = new QComboBox(lightingTab);
    m_directionCombo->addItem(tr("Right"), static_cast<int>(Direction::Right));
    m_directionCombo->addItem(tr("Left"), static_cast<int>(Direction::Left));
    m_directionCombo->addItem(tr("Up"), static_cast<int>(Direction::Up));
    m_directionCombo->addItem(tr("Down"), static_cast<int>(Direction::Down));
    form->addRow(tr("Direction:"), m_directionCombo);

    lightingLayout->addLayout(form);

    m_applyBtn = new QPushButton(tr("Apply Lighting"), lightingTab);
    m_applyBtn->setDefault(true);
    m_applyBtn->setFixedHeight(36);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    lightingLayout->addWidget(m_applyBtn);

    m_tabs->addTab(lightingTab, tr("Lighting"));

    // TAB 2: Profiles & Hardware Macros
    auto *profileTab = new QWidget();
    auto *profileLayout = new QVBoxLayout(profileTab);
    profileLayout->setContentsMargins(12, 12, 12, 12);
    profileLayout->setSpacing(14);

    auto *profGroup = new QGroupBox(tr("Hardware Profiles (Onboard Memory)"), profileTab);
    auto *profForm = new QFormLayout(profGroup);
    m_profileCombo = new QComboBox(profGroup);
    m_profileCombo->addItem(tr("Profile 1 (Default / Gaming)"), 0);
    m_profileCombo->addItem(tr("Profile 2 (Work / Productivity)"), 1);
    m_profileCombo->addItem(tr("Profile 3 (Custom)"), 2);
    m_applyProfileBtn = new QPushButton(tr("Switch Profile"), profGroup);
    connect(m_applyProfileBtn, &QPushButton::clicked, this, [this]() {
        onProfileSelected(m_profileCombo->currentIndex());
    });
    profForm->addRow(tr("Select Profile:"), m_profileCombo);
    profForm->addRow("", m_applyProfileBtn);
    profileLayout->addWidget(profGroup);

    auto *macroGroup = new QGroupBox(tr("Direct Hardware Key Macro Assignment"), profileTab);
    auto *macroForm = new QFormLayout(macroGroup);
    m_macroKeyCombo = new QComboBox(macroGroup);
    m_macroKeyCombo->addItem(tr("F1"), 0x3A);
    m_macroKeyCombo->addItem(tr("F2"), 0x3B);
    m_macroKeyCombo->addItem(tr("F3"), 0x3C);
    m_macroKeyCombo->addItem(tr("F4"), 0x3D);
    m_macroKeyCombo->addItem(tr("G (Macro Key)"), 0x0A);
    m_macroKeyCombo->addItem(tr("Insert"), 0x49);
    m_macroKeyCombo->addItem(tr("Home"), 0x4A);
    m_macroKeyCombo->addItem(tr("Page Up"), 0x4B);

    m_macroInput = new QLineEdit(macroGroup);
    m_macroInput->setPlaceholderText(tr("Key sequence or text (e.g. GG WP)"));

    m_assignMacroBtn = new QPushButton(tr("Flash Macro into Keyboard"), macroGroup);
    connect(m_assignMacroBtn, &QPushButton::clicked, this, &MainWindow::onAssignMacro);

    macroForm->addRow(tr("Target Key:"), m_macroKeyCombo);
    macroForm->addRow(tr("Sequence:"), m_macroInput);
    macroForm->addRow("", m_assignMacroBtn);
    profileLayout->addWidget(macroGroup);

    profileLayout->addStretch(1);
    m_tabs->addTab(profileTab, tr("Profiles & Macros"));

    // TAB 3: Integrations & Background Services
    auto *integrationTab = new QWidget();
    auto *integLayout = new QVBoxLayout(integrationTab);
    integLayout->setContentsMargins(12, 12, 12, 12);
    integLayout->setSpacing(14);

    // KDE Brightness
    auto *kdeGroup = new QGroupBox(tr("KDE Plasma Native Brightness Applet"), integrationTab);
    auto *kdeLayout = new QVBoxLayout(kdeGroup);
    m_kdeLedStatusLabel = new QLabel(tr("Active sysfs virtual interface:<br><code>$XDG_RUNTIME_DIR/genesis_thor404::kbd_backlight.brightness</code><br>Controlled directly by KDE PowerDevil slider & shortcuts."), kdeGroup);
    m_kdeLedStatusLabel->setWordWrap(true);
    kdeLayout->addWidget(m_kdeLedStatusLabel);
    integLayout->addWidget(kdeGroup);

    // Home Assistant MQTT
    auto *haGroup = new QGroupBox(tr("Home Assistant MQTT Discovery"), integrationTab);
    auto *haForm = new QFormLayout(haGroup);

    m_mqttHostInput = new QLineEdit("127.0.0.1", haGroup);
    m_mqttPortInput = new QSpinBox(haGroup);
    m_mqttPortInput->setRange(1, 65535);
    m_mqttPortInput->setValue(1883);
    m_mqttUserInput = new QLineEdit(haGroup);
    m_mqttUserInput->setPlaceholderText(tr("Optional username"));
    m_mqttPassInput = new QLineEdit(haGroup);
    m_mqttPassInput->setEchoMode(QLineEdit::Password);
    m_mqttPassInput->setPlaceholderText(tr("Optional password"));

    m_mqttConnectBtn = new QPushButton(tr("Connect to MQTT Broker"), haGroup);
    connect(m_mqttConnectBtn, &QPushButton::clicked, this, [this]() {
        m_mqttService.start(m_mqttHostInput->text().trimmed(),
                            static_cast<quint16>(m_mqttPortInput->value()),
                            m_mqttUserInput->text().trimmed(),
                            m_mqttPassInput->text().trimmed());
        m_mqttStatusLabel->setText(tr("Connecting to %1:%2...").arg(m_mqttHostInput->text()).arg(m_mqttPortInput->value()));
    });

    m_mqttStatusLabel = new QLabel(tr("Auto-discovery publishes to <code>homeassistant/light/thor_404/config</code>"), haGroup);
    m_mqttStatusLabel->setWordWrap(true);

    haForm->addRow(tr("MQTT Broker:"), m_mqttHostInput);
    haForm->addRow(tr("Port:"), m_mqttPortInput);
    haForm->addRow(tr("Username:"), m_mqttUserInput);
    haForm->addRow(tr("Password:"), m_mqttPassInput);
    haForm->addRow("", m_mqttConnectBtn);
    haForm->addRow(m_mqttStatusLabel);
    integLayout->addWidget(haGroup);

    integLayout->addStretch(1);
    m_tabs->addTab(integrationTab, tr("Integrations"));

    // TAB 4: Tray Quick Actions Customizer
    auto *trayTab = new QWidget();
    auto *trayLayout = new QVBoxLayout(trayTab);
    trayLayout->setContentsMargins(12, 12, 12, 12);
    trayLayout->setSpacing(14);

    for (int slotIdx = 0; slotIdx < 2; ++slotIdx) {
        auto *slotBox = new QGroupBox(tr("Right-Click Quick Action %1").arg(slotIdx + 1), trayTab);
        auto *slotForm = new QFormLayout(slotBox);

        auto *effectCombo = new QComboBox(slotBox);
        effectCombo->addItem(tr("Static"), static_cast<int>(EffectId::Static));
        effectCombo->addItem(tr("Breathing"), static_cast<int>(EffectId::Breathing));
        effectCombo->addItem(tr("Neon"), static_cast<int>(EffectId::Neon));
        effectCombo->addItem(tr("Wave"), static_cast<int>(EffectId::Wave));
        effectCombo->addItem(tr("Rainbow"), static_cast<int>(EffectId::Rainbow));
        effectCombo->addItem(tr("Spectrum"), static_cast<int>(EffectId::Spectrum));
        effectCombo->addItem(tr("Colourful"), static_cast<int>(EffectId::Colourful));
        effectCombo->addItem(tr("Circle"), static_cast<int>(EffectId::Circle));
        effectCombo->addItem(tr("Stars"), static_cast<int>(EffectId::Stars));
        effectCombo->addItem(tr("Raindrops"), static_cast<int>(EffectId::Raindrops));
        effectCombo->addItem(tr("Sine Wave"), static_cast<int>(EffectId::SineWave));

        // Select slot's current effect
        int foundEff = effectCombo->findData(static_cast<int>(m_quickSlots[slotIdx].effect));
        if (foundEff >= 0) effectCombo->setCurrentIndex(foundEff);

        auto *colorRow = new QHBoxLayout();
        auto *colorBtn = new QPushButton(m_quickSlots[slotIdx].color.name().toUpper(), slotBox);
        auto *multiCheck = new QCheckBox(tr("Rainbow"), slotBox);
        multiCheck->setChecked(m_quickSlots[slotIdx].multicolor);

        QPixmap pix(16, 16);
        pix.fill(m_quickSlots[slotIdx].color);
        colorBtn->setIcon(QIcon(pix));

        connect(colorBtn, &QPushButton::clicked, this, [this, slotIdx, colorBtn]() {
            QColor c = QColorDialog::getColor(m_quickSlots[slotIdx].color, this, tr("Select Slot Color"));
            if (c.isValid()) {
                m_quickSlots[slotIdx].color = c;
                QPixmap p(16, 16);
                p.fill(c);
                colorBtn->setIcon(QIcon(p));
                colorBtn->setText(c.name().toUpper());
                updateTrayMenuText();
            }
        });

        connect(effectCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, slotIdx, effectCombo]() {
            m_quickSlots[slotIdx].effect = static_cast<EffectId>(effectCombo->currentData().toInt());
            updateTrayMenuText();
        });

        connect(multiCheck, &QCheckBox::toggled, this, [this, slotIdx](bool checked) {
            m_quickSlots[slotIdx].multicolor = checked;
            updateTrayMenuText();
        });

        colorRow->addWidget(colorBtn);
        colorRow->addWidget(multiCheck);

        slotForm->addRow(tr("Effect:"), effectCombo);
        slotForm->addRow(tr("Color:"), colorRow);
        trayLayout->addWidget(slotBox);
    }

    auto *toggleInfo = new QLabel(tr("<b>Toggle Action:</b> Right-clicking the tray icon and selecting <i>Toggle Off / On</i> turns the backlight on or off, remembering your previous preset."), trayTab);
    toggleInfo->setWordWrap(true);
    trayLayout->addWidget(toggleInfo);
    trayLayout->addStretch(1);

    m_tabs->addTab(trayTab, tr("Tray Actions"));

    mainLayout->addWidget(m_tabs);

    // Status Message footer
    m_msgLabel = new QLabel(central);
    m_msgLabel->setAlignment(Qt::AlignCenter);
    m_msgLabel->setWordWrap(true);
    mainLayout->addWidget(m_msgLabel);

    updateStatusBadge(false);
}

void MainWindow::setupTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayMenu = new QMenu(this);

    auto *showAction = m_trayMenu->addAction(tr("Open Window"));
    connect(showAction, &QAction::triggered, this, &MainWindow::showNormal);

    m_trayMenu->addSeparator();

    // Adjustable quick action slot 1 (Default: Red Static)
    m_quickAction1 = m_trayMenu->addAction("");
    connect(m_quickAction1, &QAction::triggered, this, [this]() {
        const auto &slot = m_quickSlots[0];
        EffectConfig cfg;
        cfg.effect = slot.effect;
        cfg.r = static_cast<uint8_t>(slot.color.red());
        cfg.g = static_cast<uint8_t>(slot.color.green());
        cfg.b = static_cast<uint8_t>(slot.color.blue());
        cfg.multicolor = slot.multicolor;
        cfg.brightness = static_cast<uint8_t>(m_brightnessSlider->value());
        cfg.speed = static_cast<uint8_t>(m_speedSlider->value());
        cfg.direction = static_cast<Direction>(m_directionCombo->currentData().toInt());
        m_isLightOn = true;
        applyConfig(cfg);
        updateTrayMenuText();
    });

    // Adjustable quick action slot 2 (Default: Wave Rainbow)
    m_quickAction2 = m_trayMenu->addAction("");
    connect(m_quickAction2, &QAction::triggered, this, [this]() {
        const auto &slot = m_quickSlots[1];
        EffectConfig cfg;
        cfg.effect = slot.effect;
        cfg.r = static_cast<uint8_t>(slot.color.red());
        cfg.g = static_cast<uint8_t>(slot.color.green());
        cfg.b = static_cast<uint8_t>(slot.color.blue());
        cfg.multicolor = slot.multicolor;
        cfg.brightness = static_cast<uint8_t>(m_brightnessSlider->value());
        cfg.speed = static_cast<uint8_t>(m_speedSlider->value());
        cfg.direction = static_cast<Direction>(m_directionCombo->currentData().toInt());
        m_isLightOn = true;
        applyConfig(cfg);
        updateTrayMenuText();
    });

    m_trayMenu->addSeparator();

    // Toggle lighting action (Toggle Off / On)
    m_toggleAction = m_trayMenu->addAction("");
    connect(m_toggleAction, &QAction::triggered, this, &MainWindow::onToggleLighting);

    m_trayMenu->addSeparator();

    auto *quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);

    updateTrayMenuText();
    updateTrayIcon();
    m_trayIcon->show();
}

static QString effectName(EffectId eff) {
    switch (eff) {
        case EffectId::Static: return "Static";
        case EffectId::Breathing: return "Breathing";
        case EffectId::Neon: return "Neon";
        case EffectId::Wave: return "Wave";
        case EffectId::Rainbow: return "Rainbow";
        case EffectId::Spectrum: return "Spectrum";
        case EffectId::Colourful: return "Colourful";
        case EffectId::Circle: return "Circle";
        case EffectId::Stars: return "Stars";
        case EffectId::Raindrops: return "Raindrops";
        case EffectId::SineWave: return "Sine Wave";
        case EffectId::Off: return "Off";
        default: return "Custom";
    }
}

void MainWindow::updateTrayMenuText() {
    if (m_quickAction1) {
        const auto &s1 = m_quickSlots[0];
        QString desc1 = s1.multicolor ? QString("%1 (Rainbow)").arg(effectName(s1.effect))
                                      : QString("%1 (%2)").arg(effectName(s1.effect)).arg(s1.color.name().toUpper());
        m_quickAction1->setText(tr("Preset 1: %1").arg(desc1));

        QPixmap p1(12, 12);
        p1.fill(s1.multicolor ? Qt::magenta : s1.color);
        m_quickAction1->setIcon(QIcon(p1));
    }

    if (m_quickAction2) {
        const auto &s2 = m_quickSlots[1];
        QString desc2 = s2.multicolor ? QString("%1 (Rainbow)").arg(effectName(s2.effect))
                                      : QString("%1 (%2)").arg(effectName(s2.effect)).arg(s2.color.name().toUpper());
        m_quickAction2->setText(tr("Preset 2: %1").arg(desc2));

        QPixmap p2(12, 12);
        p2.fill(s2.multicolor ? Qt::cyan : s2.color);
        m_quickAction2->setIcon(QIcon(p2));
    }

    if (m_toggleAction) {
        m_toggleAction->setText(m_isLightOn ? tr("Toggle Lighting (Turn Off)") : tr("Toggle Lighting (Turn On)"));
    }
}

void MainWindow::onToggleLighting() {
    if (m_isLightOn) {
        EffectConfig cfg;
        cfg.effect = EffectId::Off;
        applyConfig(cfg);
        m_isLightOn = false;
    } else {
        // Turn back on with current lighting settings
        onApply();
        m_isLightOn = true;
    }
    updateTrayMenuText();
}

void MainWindow::updateTrayIcon() {
    // Dynamic monochrome icon adapting to KDE Plasma Light / Dark palette
    QColor textColor = palette().color(QPalette::WindowText);
    bool isDarkTheme = (textColor.lightness() > 128); // Text is light -> Dark theme

    QPixmap pix(22, 22);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QColor iconColor = isDarkTheme ? QColor(240, 240, 240) : QColor(30, 30, 30);
    p.setPen(QPen(iconColor, 1.8));

    // Draw keyboard icon contour
    p.drawRoundedRect(2, 4, 18, 14, 2, 2);

    // Draw key grid lines
    p.drawLine(5, 8, 8, 8);
    p.drawLine(10, 8, 13, 8);
    p.drawLine(15, 8, 17, 8);

    p.drawLine(5, 11, 7, 11);
    p.drawLine(9, 11, 13, 11);
    p.drawLine(15, 11, 17, 11);

    // Spacebar
    p.drawLine(7, 14, 15, 14);

    p.end();

    m_trayIcon->setIcon(QIcon(pix));
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
    }
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
        m_statusBadge->setStyleSheet("color: #388e3c; font-weight: bold; padding: 2px;");
        if (m_msgLabel) m_msgLabel->clear();
    } else {
        auto devs = m_deviceManager.enumerateDevices();
        if (!devs.empty()) {
            m_statusBadge->setText(tr("○ Permission Denied"));
            m_statusBadge->setStyleSheet("color: #f57c00; font-weight: bold; padding: 2px;");
            if (m_msgLabel) {
                m_msgLabel->setText(tr("Device detected on USB but hidraw permissions are missing.<br>Run: <code>sudo cp 99-genesis-thor.rules /etc/udev/rules.d/ && sudo udevadm trigger</code>"));
            }
        } else {
            m_statusBadge->setText(tr("○ Disconnected"));
            m_statusBadge->setStyleSheet("color: #d32f2f; font-weight: bold; padding: 2px;");
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

void MainWindow::applyConfig(const EffectConfig &cfg) {
    if (!m_deviceManager.isConnected()) {
        int targetPid = m_modelCombo->currentData().toInt();
        m_deviceManager.openDevice(targetPid);
        updateStatusBadge(m_deviceManager.isConnected());
    }

    if (!m_deviceManager.isConnected()) {
        if (m_msgLabel) m_msgLabel->setText(tr("Device not found. Check USB connection and udev rules."));
        return;
    }

    std::string err;
    if (m_deviceManager.applyEffect(cfg, &err)) {
        if (m_msgLabel) m_msgLabel->setText(tr("Settings applied successfully!"));
        m_mqttService.publishState(cfg.effect != EffectId::Off, cfg.brightness,
                                   QColor(cfg.r, cfg.g, cfg.b), "active");
        m_sysfsLedService.updateBrightness(cfg.brightness);
    } else {
        if (m_msgLabel) m_msgLabel->setText(tr("Failed to apply settings: %1").arg(QString::fromStdString(err)));
    }
}

void MainWindow::onApply() {
    EffectConfig cfg;
    cfg.effect = static_cast<EffectId>(m_effectCombo->currentData().toInt());
    cfg.r = static_cast<uint8_t>(m_selectedColor.red());
    cfg.g = static_cast<uint8_t>(m_selectedColor.green());
    cfg.b = static_cast<uint8_t>(m_selectedColor.blue());
    cfg.multicolor = m_multicolorCheck->isChecked();
    cfg.brightness = static_cast<uint8_t>(m_brightnessSlider->value());
    cfg.speed = static_cast<uint8_t>(m_speedSlider->value());
    cfg.direction = static_cast<Direction>(m_directionCombo->currentData().toInt());

    applyConfig(cfg);
}

void MainWindow::onProfileSelected(int index) {
    if (!m_deviceManager.isConnected()) {
        m_msgLabel->setText(tr("Cannot switch profile: device disconnected"));
        return;
    }
    std::string err;
    if (m_deviceManager.setProfile(static_cast<uint8_t>(index), &err)) {
        m_msgLabel->setText(tr("Switched to Hardware Profile %1 (stored in keyboard memory)").arg(index + 1));
    } else {
        m_msgLabel->setText(tr("Failed to switch profile: %1").arg(QString::fromStdString(err)));
    }
}

void MainWindow::onAssignMacro() {
    if (!m_deviceManager.isConnected()) {
        m_msgLabel->setText(tr("Cannot assign macro: device disconnected"));
        return;
    }
    uint8_t key = static_cast<uint8_t>(m_macroKeyCombo->currentData().toInt());
    QString text = m_macroInput->text();
    if (text.isEmpty()) {
        m_msgLabel->setText(tr("Please enter a macro sequence"));
        return;
    }

    std::vector<uint8_t> scancodes;
    for (QChar c : text) {
        scancodes.push_back(static_cast<uint8_t>(c.toLatin1()));
    }

    std::string err;
    if (m_deviceManager.assignKeyMacro(key, scancodes, &err)) {
        m_msgLabel->setText(tr("Flashed macro sequence directly into keyboard memory!"));
        m_macroInput->clear();
    } else {
        m_msgLabel->setText(tr("Failed to flash macro: %1").arg(QString::fromStdString(err)));
    }
}

} // namespace thor
