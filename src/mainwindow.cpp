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
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QIcon>

namespace thor {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle(tr("Genesis Thor 404 TKL"));
    setWindowIcon(QIcon(":/resources/genesis-logo.svg"));
    resize(740, 620);
    setMinimumWidth(680);

    // Initialize default quick action slots:
    // Slot 1: Static Red, Slot 2: Static Blue, Slot 3: Wave Rainbow
    m_quickSlots[0] = { tr("Red Static"), EffectId::Static, QColor(255, 0, 0), false };
    m_quickSlots[1] = { tr("Blue Static"), EffectId::Static, QColor(0, 0, 255), false };
    m_quickSlots[2] = { tr("Wave Rainbow"), EffectId::Wave, QColor(255, 0, 0), true };

    setupUi();
    setupTray();

    // Connect Home Assistant MQTT apply callback
    m_mqttService.setApplyCallback([this](const EffectConfig &cfg) {
        // Sync local GUI controls with Home Assistant
        m_selectedColor = QColor(cfg.r, cfg.g, cfg.b);
        updateSwatch();
        if (m_lightingVisualizer) {
            m_lightingVisualizer->setCurrentBrushColor(m_selectedColor);
            m_lightingVisualizer->setAllKeysColor(m_selectedColor);
        }
        if (m_brightnessSlider) {
            m_brightnessSlider->setValue(cfg.brightness);
        }
        if (m_effectCombo) {
            int idx = m_effectCombo->findData(static_cast<int>(cfg.effect));
            if (idx >= 0) {
                m_effectCombo->setCurrentIndex(idx);
            }
        }
        applyConfig(cfg);
    });

    connect(&m_mqttService, &MqttLightService::connectedChanged, this, [this](bool connected, const QString &message) {
        if (m_mqttStatusLabel) {
            QString color = connected ? "#388e3c" : "#d32f2f";
            m_mqttStatusLabel->setText(QString("<b style='color:%1;'>%2</b><br><small>Auto-discovery: <code>homeassistant/light/thor_404/config</code></small>").arg(color, message));
        }
        if (connected) {
            // Save successful settings for autoconnect
            QSettings settings("Genesis", "Thor404Controller");
            settings.setValue("mqtt/host", m_mqttHostInput->text().trimmed());
            settings.setValue("mqtt/port", m_mqttPortInput->value());
            settings.setValue("mqtt/user", m_mqttUserInput->text().trimmed());
            settings.setValue("mqtt/pass", m_mqttPassInput->text().trimmed());
            settings.setValue("mqtt/autoConnect", m_mqttAutoConnectCheck->isChecked());
        }
    });

    // Load persisted settings
    QSettings settings("Genesis", "Thor404Controller");
    QString savedHost = settings.value("mqtt/host", "127.0.0.1").toString();
    int savedPort = settings.value("mqtt/port", 1883).toInt();
    QString savedUser = settings.value("mqtt/user", "").toString();
    QString savedPass = settings.value("mqtt/pass", "").toString();
    bool autoConnect = settings.value("mqtt/autoConnect", false).toBool();

    m_mqttHostInput->setText(savedHost);
    m_mqttPortInput->setValue(savedPort);
    m_mqttUserInput->setText(savedUser);
    m_mqttPassInput->setText(savedPass);
    m_mqttAutoConnectCheck->setChecked(autoConnect);

    if (autoConnect) {
        m_mqttService.start(savedHost, static_cast<quint16>(savedPort), savedUser, savedPass);
        m_mqttStatusLabel->setText(tr("Auto-connecting to %1:%2...").arg(savedHost).arg(savedPort));
    }

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

    m_lightingFormLayout = new QFormLayout();
    m_lightingFormLayout->setSpacing(10);

    // Effect
    m_effectCombo = new QComboBox(lightingTab);
    m_effectCombo->addItem(tr("Static"), static_cast<int>(EffectId::Static));
    m_effectCombo->addItem(tr("Customize (Per-Key RGB)"), static_cast<int>(EffectId::Custom));
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
    m_lightingFormLayout->addRow(tr("Effect:"), m_effectCombo);

    // Interactive Keyboard Visualizer
    m_lightingVisualizer = new InteractiveKeyboardWidget(lightingTab);
    m_lightingVisualizer->setMode(InteractiveKeyboardWidget::Mode::ColorPicker);
    m_lightingVisualizer->setCurrentBrushColor(m_selectedColor);
    lightingLayout->addWidget(m_lightingVisualizer);

    m_customActionsContainer = new QWidget(lightingTab);
    auto *visActionLayout = new QHBoxLayout(m_customActionsContainer);
    visActionLayout->setContentsMargins(0, 0, 0, 0);
    m_fillAllBtn = new QPushButton(tr("Fill All With Brush"), m_customActionsContainer);
    connect(m_fillAllBtn, &QPushButton::clicked, this, [this]() {
        m_lightingVisualizer->setAllKeysColor(m_selectedColor);
    });
    visActionLayout->addWidget(m_fillAllBtn);

    auto *wasdBtn = new QPushButton(tr("Highlight WASD"), m_customActionsContainer);
    connect(wasdBtn, &QPushButton::clicked, this, [this]() {
        m_lightingVisualizer->setKeyColor("w", m_selectedColor);
        m_lightingVisualizer->setKeyColor("a", m_selectedColor);
        m_lightingVisualizer->setKeyColor("s", m_selectedColor);
        m_lightingVisualizer->setKeyColor("d", m_selectedColor);
    });
    visActionLayout->addWidget(wasdBtn);

    auto *arrowBtn = new QPushButton(tr("Highlight Arrows"), m_customActionsContainer);
    connect(arrowBtn, &QPushButton::clicked, this, [this]() {
        m_lightingVisualizer->setKeyColor("up arrow", m_selectedColor);
        m_lightingVisualizer->setKeyColor("down arrow", m_selectedColor);
        m_lightingVisualizer->setKeyColor("left arrow", m_selectedColor);
        m_lightingVisualizer->setKeyColor("right arrow", m_selectedColor);
    });
    visActionLayout->addWidget(arrowBtn);

    visActionLayout->addStretch(1);
    lightingLayout->addWidget(m_customActionsContainer);

    // Color picker
    m_colorContainer = new QWidget(lightingTab);
    auto *colorRowLayout = new QHBoxLayout(m_colorContainer);
    colorRowLayout->setContentsMargins(0, 0, 0, 0);
    m_colorBtn = new QPushButton(tr("Select Color..."), m_colorContainer);
    updateSwatch();
    connect(m_colorBtn, &QPushButton::clicked, this, &MainWindow::onPickColor);

    m_multicolorCheck = new QCheckBox(tr("Multicolor / Rainbow"), m_colorContainer);
    connect(m_multicolorCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_lightingVisualizer) {
            auto eff = static_cast<EffectId>(m_effectCombo->currentData().toInt());
            if (eff != EffectId::Custom && eff != EffectId::Off) {
                if (checked) {
                    m_lightingVisualizer->setRainbowEffect();
                } else {
                    m_lightingVisualizer->setAllKeysColor(m_selectedColor);
                }
            }
        }
    });
    colorRowLayout->addWidget(m_colorBtn);
    colorRowLayout->addWidget(m_multicolorCheck);
    m_lightingFormLayout->addRow(tr("Color:"), m_colorContainer);

    // Brightness slider (1 - 16)
    m_brightnessContainer = new QWidget(lightingTab);
    auto *brightLayout = new QHBoxLayout(m_brightnessContainer);
    brightLayout->setContentsMargins(0, 0, 0, 0);
    m_brightnessSlider = new QSlider(Qt::Horizontal, m_brightnessContainer);
    m_brightnessSlider->setRange(1, 16);
    m_brightnessSlider->setValue(16);
    m_brightnessValLabel = new QLabel(QString::number(m_brightnessSlider->value()), m_brightnessContainer);
    m_brightnessValLabel->setFixedWidth(24);
    connect(m_brightnessSlider, &QSlider::valueChanged, this, [this](int val) {
        m_brightnessValLabel->setText(QString::number(val));
    });
    brightLayout->addWidget(m_brightnessSlider);
    brightLayout->addWidget(m_brightnessValLabel);
    m_lightingFormLayout->addRow(tr("Brightness:"), m_brightnessContainer);

    // Speed slider (1 - 16)
    m_speedContainer = new QWidget(lightingTab);
    auto *speedLayout = new QHBoxLayout(m_speedContainer);
    speedLayout->setContentsMargins(0, 0, 0, 0);
    m_speedSlider = new QSlider(Qt::Horizontal, m_speedContainer);
    m_speedSlider->setRange(1, 16);
    m_speedSlider->setValue(16);
    m_speedValLabel = new QLabel(QString::number(m_speedSlider->value()), m_speedContainer);
    m_speedValLabel->setFixedWidth(24);
    connect(m_speedSlider, &QSlider::valueChanged, this, [this](int val) {
        m_speedValLabel->setText(QString::number(val));
    });
    speedLayout->addWidget(m_speedSlider);
    speedLayout->addWidget(m_speedValLabel);
    m_lightingFormLayout->addRow(tr("Speed:"), m_speedContainer);

    // Direction dropdown
    m_directionContainer = new QWidget(lightingTab);
    auto *dirLayout = new QHBoxLayout(m_directionContainer);
    dirLayout->setContentsMargins(0, 0, 0, 0);
    m_directionCombo = new QComboBox(m_directionContainer);
    m_directionCombo->addItem(tr("Right"), static_cast<int>(Direction::Right));
    m_directionCombo->addItem(tr("Left"), static_cast<int>(Direction::Left));
    m_directionCombo->addItem(tr("Up"), static_cast<int>(Direction::Up));
    m_directionCombo->addItem(tr("Down"), static_cast<int>(Direction::Down));
    dirLayout->addWidget(m_directionCombo);
    m_lightingFormLayout->addRow(tr("Direction:"), m_directionContainer);

    lightingLayout->addLayout(m_lightingFormLayout);

    m_applyBtn = new QPushButton(tr("Apply Lighting"), lightingTab);
    m_applyBtn->setDefault(true);
    m_applyBtn->setFixedHeight(36);
    connect(m_applyBtn, &QPushButton::clicked, this, &MainWindow::onApply);
    lightingLayout->addWidget(m_applyBtn);

    m_tabs->addTab(lightingTab, tr("Lighting"));

    // Trigger initial visibility state for default selected effect (Static)
    onEffectChanged(m_effectCombo->currentIndex());

    // TAB 2: Integrations & Background Services
    auto *integrationTab = new QWidget();
    auto *integLayout = new QVBoxLayout(integrationTab);
    integLayout->setContentsMargins(12, 12, 12, 12);
    integLayout->setSpacing(14);

    // System / Autostart Group
    auto *sysGroup = new QGroupBox(tr("System Startup & Daemon"), integrationTab);
    auto *sysLayout = new QVBoxLayout(sysGroup);
    m_autostartCheck = new QCheckBox(tr("Start Thor 404 Controller automatically on system login"), sysGroup);
    
    // Check if autostart desktop entry exists
    QString autostartPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/thor-404-controller.desktop";
    m_autostartCheck->setChecked(QFile::exists(autostartPath));
    connect(m_autostartCheck, &QCheckBox::toggled, this, &MainWindow::updateAutostart);
    sysLayout->addWidget(m_autostartCheck);
    integLayout->addWidget(sysGroup);

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
    m_mqttAutoConnectCheck = new QCheckBox(tr("Auto-connect to MQTT on startup"), haGroup);

    m_mqttConnectBtn = new QPushButton(tr("Connect to MQTT Broker"), haGroup);
    connect(m_mqttConnectBtn, &QPushButton::clicked, this, [this]() {
        QSettings s("Genesis", "Thor404Controller");
        s.setValue("mqtt/autoConnect", m_mqttAutoConnectCheck->isChecked());
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
    haForm->addRow("", m_mqttAutoConnectCheck);
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

    for (int slotIdx = 0; slotIdx < 3; ++slotIdx) {
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

    // Adjustable quick action slot 2 (Default: Blue Static)
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

    // Adjustable quick action slot 3 (Default: Wave Rainbow)
    m_quickAction3 = m_trayMenu->addAction("");
    connect(m_quickAction3, &QAction::triggered, this, [this]() {
        const auto &slot = m_quickSlots[2];
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

    if (m_quickAction3) {
        const auto &s3 = m_quickSlots[2];
        QString desc3 = s3.multicolor ? QString("%1 (Rainbow)").arg(effectName(s3.effect))
                                      : QString("%1 (%2)").arg(effectName(s3.effect)).arg(s3.color.name().toUpper());
        m_quickAction3->setText(tr("Preset 3: %1").arg(desc3));

        QPixmap p3(12, 12);
        p3.fill(s3.multicolor ? Qt::cyan : s3.color);
        m_quickAction3->setIcon(QIcon(p3));
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
    bool isCustom = (eff == EffectId::Custom);
    bool isStatic = (eff == EffectId::Static);
    bool hasDirection = (eff == EffectId::Wave || eff == EffectId::Shuttle || eff == EffectId::Shockwave);
    bool hasSpeed = (!isOff && !isStatic && !isCustom);
    bool hasMulticolor = (!isOff && !isStatic && !isCustom);

    // Only allow clicking/dragging individual key colors when in Custom mode
    if (m_lightingVisualizer) {
        m_lightingVisualizer->setInteractive(isCustom);

        if (isOff) {
            m_lightingVisualizer->setAllKeysColor(QColor(0, 0, 0));
        } else if (eff == EffectId::Rainbow || eff == EffectId::Spectrum || eff == EffectId::Colourful
                   || (m_multicolorCheck && m_multicolorCheck->isChecked())) {
            m_lightingVisualizer->setRainbowEffect();
        } else if (!isCustom) {
            m_lightingVisualizer->setAllKeysColor(m_selectedColor);
        }
    }

    // Show custom actions (Fill All, WASD, Arrows) only in Custom mode
    if (m_customActionsContainer) {
        m_customActionsContainer->setVisible(isCustom);
    }

    if (m_lightingFormLayout) {
        m_lightingFormLayout->setRowVisible(m_colorContainer, !isOff);
        if (m_multicolorCheck) {
            m_multicolorCheck->setVisible(hasMulticolor);
        }
        m_lightingFormLayout->setRowVisible(m_brightnessContainer, !isOff);
        m_lightingFormLayout->setRowVisible(m_speedContainer, hasSpeed);
        m_lightingFormLayout->setRowVisible(m_directionContainer, hasDirection);
    }
}

void MainWindow::onPickColor() {
    QColor chosen = QColorDialog::getColor(m_selectedColor, this, tr("Select Lighting Color"));
    if (chosen.isValid()) {
        m_selectedColor = chosen;
        updateSwatch();
        if (m_lightingVisualizer) {
            m_lightingVisualizer->setCurrentBrushColor(m_selectedColor);
            auto eff = static_cast<EffectId>(m_effectCombo->currentData().toInt());
            if (eff != EffectId::Custom && eff != EffectId::Off) {
                if (m_multicolorCheck && m_multicolorCheck->isChecked()) {
                    m_lightingVisualizer->setRainbowEffect();
                } else {
                    m_lightingVisualizer->setAllKeysColor(m_selectedColor);
                }
            }
        }
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
    } else {
        if (m_msgLabel) m_msgLabel->setText(tr("Failed to apply settings: %1").arg(QString::fromStdString(err)));
    }
}

void MainWindow::onApply() {
    auto eff = static_cast<EffectId>(m_effectCombo->currentData().toInt());
    if (eff == EffectId::Custom) {
        onApplyCustomLighting();
        return;
    }

    EffectConfig cfg;
    cfg.effect = eff;
    cfg.r = static_cast<uint8_t>(m_selectedColor.red());
    cfg.g = static_cast<uint8_t>(m_selectedColor.green());
    cfg.b = static_cast<uint8_t>(m_selectedColor.blue());
    cfg.multicolor = m_multicolorCheck->isChecked();
    cfg.brightness = static_cast<uint8_t>(m_brightnessSlider->value());
    cfg.speed = static_cast<uint8_t>(m_speedSlider->value());
    cfg.direction = static_cast<Direction>(m_directionCombo->currentData().toInt());

    applyConfig(cfg);
}

void MainWindow::onApplyCustomLighting() {
    if (!m_deviceManager.isConnected()) {
        int targetPid = m_modelCombo->currentData().toInt();
        m_deviceManager.openDevice(targetPid);
        updateStatusBadge(m_deviceManager.isConnected());
    }

    if (!m_deviceManager.isConnected()) {
        if (m_msgLabel) m_msgLabel->setText(tr("Device not found. Check USB connection."));
        return;
    }

    // Convert visualizer key map to line/dword packet map
    std::map<std::pair<uint8_t, uint8_t>, std::array<uint8_t, 3>> keyRgbMap;
    if (m_lightingVisualizer) {
        auto colors = m_lightingVisualizer->getKeyColors();
        // Look up key coordinate definitions
        // InteractiveKeyboardWidget has the keys
        // Let's populate the active key colors
        for (const auto& [id, col] : colors) {
            // Find key definition
            // Default ISO-88 key matrix mappings:
            // Esc: 12, 1; F1-F12: 12, 2..13; Prt, Scr, Pau: 19, 0..3
            // Number row: 13, 3..15; Backspace: 18, 7; Ins, Hom, PgUp: 19, 4..6
            // Q row: 14, 5..15; [ ]: 15, 0..1; Del, End, PgDn: 19, 7..9
            // Caps, A..L, ;, ', #: 15, 7..15; 16, 0..3; Enter: 17, 5
            // LShift, \, Z..M, ,, ., /: 16, 9, 3, 10..15; 17, 0..3; RShift: 17, 4; Up: 18, 5
            // Ctrl, Win, Alt, Space, AltGr, Fn, Menu, RCtrl: 17, 11..15; 18, 0..2; Arrows: 18, 3,4,6
            static const std::map<std::string, std::pair<uint8_t, uint8_t>> idCoords = {
                {"escape", {12, 1}}, {"f1", {12, 2}}, {"f2", {12, 3}}, {"f3", {12, 4}}, {"f4", {12, 5}},
                {"f5", {12, 6}}, {"f6", {12, 7}}, {"f7", {12, 8}}, {"f8", {12, 9}}, {"f9", {12, 10}},
                {"f10", {12, 11}}, {"f11", {12, 12}}, {"f12", {12, 13}},
                {"prtsc", {19, 0}}, {"scrllk", {19, 1}}, {"pause", {19, 3}},
                {"backtick", {13, 3}}, {"1", {13, 4}}, {"2", {13, 5}}, {"3", {13, 6}}, {"4", {13, 7}},
                {"5", {13, 8}}, {"6", {13, 9}}, {"7", {13, 10}}, {"8", {13, 11}}, {"9", {13, 12}},
                {"0", {13, 13}}, {"-", {13, 14}}, {"=", {13, 15}}, {"backspace", {18, 7}},
                {"insert", {19, 4}}, {"home", {19, 5}}, {"page up", {19, 6}},
                {"tab", {14, 5}}, {"q", {14, 6}}, {"w", {14, 7}}, {"e", {14, 8}}, {"r", {14, 9}},
                {"t", {14, 10}}, {"y", {14, 11}}, {"u", {14, 12}}, {"i", {14, 13}}, {"o", {14, 14}},
                {"p", {14, 15}}, {"[", {15, 0}}, {"]", {15, 1}},
                {"del", {19, 7}}, {"end", {19, 8}}, {"page down", {19, 9}},
                {"caps lock", {15, 7}}, {"a", {15, 8}}, {"s", {15, 9}}, {"d", {15, 10}}, {"f", {15, 11}},
                {"g", {15, 12}}, {"h", {15, 13}}, {"j", {15, 14}}, {"k", {15, 15}}, {"l", {16, 0}},
                {";", {16, 1}}, {"'", {16, 2}}, {"reverse slash", {16, 3}}, {"enter", {17, 5}},
                {"left shift", {16, 9}}, {"iso_bslash", {16, 3}}, {"z", {16, 10}}, {"x", {16, 11}},
                {"c", {16, 12}}, {"v", {16, 13}}, {"b", {16, 14}}, {"n", {16, 15}}, {"m", {17, 0}},
                {",", {17, 1}}, {".", {17, 2}}, {"slash", {17, 3}}, {"right shift", {17, 4}},
                {"up arrow", {18, 5}},
                {"left ctrl", {17, 11}}, {"windows", {17, 12}}, {"left alt", {17, 13}}, {"space", {17, 14}},
                {"right alt", {17, 15}}, {"function", {18, 0}}, {"menu", {18, 1}}, {"right ctrl", {18, 2}},
                {"left arrow", {18, 3}}, {"down arrow", {18, 4}}, {"right arrow", {18, 6}}
            };
            auto it = idCoords.find(id);
            if (it != idCoords.end()) {
                keyRgbMap[it->second] = {static_cast<uint8_t>(col.red()),
                                         static_cast<uint8_t>(col.green()),
                                         static_cast<uint8_t>(col.blue())};
            }
        }
    }

    uint8_t brightness = static_cast<uint8_t>(m_brightnessSlider->value());
    std::string err;
    if (m_deviceManager.applyCustomEffect(keyRgbMap, brightness, &err)) {
        if (m_msgLabel) m_msgLabel->setText(tr("Custom per-key lighting sent to keyboard!"));
    } else {
        if (m_msgLabel) m_msgLabel->setText(tr("Failed to apply custom lighting: %1").arg(QString::fromStdString(err)));
    }
}

void MainWindow::updateAutostart(bool enable) {
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);
    QString filePath = autostartDir + "/thor-404-controller.desktop";

    if (enable) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString desktopContent =
                "[Desktop Entry]\n"
                "Type=Application\n"
                "Name=Genesis Thor 404 Controller\n"
                "Comment=Background RGB and Brightness Controller for Genesis Thor 404 TKL\n"
                "Exec=thor-404-controller\n"
                "Icon=thor-404-controller\n"
                "Terminal=false\n"
                "Categories=Utility;HardwareSettings;\n"
                "X-GNOME-Autostart-enabled=true\n";
            file.write(desktopContent.toUtf8());
            file.close();
        }
    } else {
        QFile::remove(filePath);
    }
}

} // namespace thor
