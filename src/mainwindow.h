#pragma once

#include <QMainWindow>
#include <QColor>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <memory>
#include "devicemanager.h"
#include "mqttservice.h"
#include "sysfsledservice.h"
#include "interactivekeyboardwidget.h"

class QLabel;
class QComboBox;
class QPushButton;
class QSlider;
class QCheckBox;
class QTimer;
class QTabWidget;
class QLineEdit;
class QSpinBox;

namespace thor {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onRefreshDevices();
    void onModelSelected(int index);
    void onEffectChanged(int index);
    void onPickColor();
    void onApply();
    void onApplyCustomLighting();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void updateTrayIcon();
    void updateTrayMenuText();
    void onToggleLighting();

private:
    void setupUi();
    void setupTray();
    void updateStatusBadge(bool connected);
    void updateSwatch();
    void applyConfig(const EffectConfig &cfg);

    DeviceManager m_deviceManager;
    MqttLightService m_mqttService;
    SysfsLedService m_sysfsLedService;

    QColor m_selectedColor{255, 0, 0}; // default red
    QSystemTrayIcon *m_trayIcon{nullptr};
    QMenu *m_trayMenu{nullptr};

    // Tray Quick Actions Configuration
    struct QuickSlot {
        QString name;
        EffectId effect{EffectId::Static};
        QColor color{255, 0, 0};
        bool multicolor{false};
    };
    std::array<QuickSlot, 3> m_quickSlots;
    bool m_isLightOn{true};

    QAction *m_quickAction1{nullptr};
    QAction *m_quickAction2{nullptr};
    QAction *m_quickAction3{nullptr};
    QAction *m_toggleAction{nullptr};

    // UI elements
    QLabel *m_statusBadge{nullptr};
    QComboBox *m_modelCombo{nullptr};
    QTabWidget *m_tabs{nullptr};

    // Lighting Tab
    InteractiveKeyboardWidget *m_lightingVisualizer{nullptr};
    QWidget *m_customActionsContainer{nullptr};
    QPushButton *m_fillAllBtn{nullptr};
    QPushButton *m_applyCustomBtn{nullptr};
    QComboBox *m_effectCombo{nullptr};
    QPushButton *m_colorBtn{nullptr};
    QCheckBox *m_multicolorCheck{nullptr};
    QWidget *m_colorContainer{nullptr};
    QWidget *m_brightnessContainer{nullptr};
    QSlider *m_brightnessSlider{nullptr};
    QLabel *m_brightnessValLabel{nullptr};
    QWidget *m_speedContainer{nullptr};
    QSlider *m_speedSlider{nullptr};
    QLabel *m_speedValLabel{nullptr};
    QWidget *m_directionContainer{nullptr};
    QComboBox *m_directionCombo{nullptr};
    QPushButton *m_applyBtn{nullptr};

    // Home Assistant / Daemon Tab
    QLineEdit *m_mqttHostInput{nullptr};
    QSpinBox *m_mqttPortInput{nullptr};
    QLineEdit *m_mqttUserInput{nullptr};
    QLineEdit *m_mqttPassInput{nullptr};
    QCheckBox *m_mqttAutoConnectCheck{nullptr};
    QPushButton *m_mqttConnectBtn{nullptr};
    QLabel *m_mqttStatusLabel{nullptr};
    QLabel *m_kdeLedStatusLabel{nullptr};

    // Autostart Setting
    QCheckBox *m_autostartCheck{nullptr};
    void updateAutostart(bool enable);

    QLabel *m_msgLabel{nullptr};
    QTimer *m_pollTimer{nullptr};
};

} // namespace thor
