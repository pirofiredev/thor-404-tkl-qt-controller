#pragma once

#include <QMainWindow>
#include <QColor>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <memory>
#include "devicemanager.h"
#include "mqttservice.h"
#include "sysfsledservice.h"

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
    void onProfileSelected(int index);
    void onAssignMacro();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void updateTrayIcon();

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

    // UI elements
    QLabel *m_statusBadge{nullptr};
    QComboBox *m_modelCombo{nullptr};
    QTabWidget *m_tabs{nullptr};

    // Lighting Tab
    QComboBox *m_effectCombo{nullptr};
    QPushButton *m_colorBtn{nullptr};
    QCheckBox *m_multicolorCheck{nullptr};
    QSlider *m_brightnessSlider{nullptr};
    QLabel *m_brightnessValLabel{nullptr};
    QSlider *m_speedSlider{nullptr};
    QLabel *m_speedValLabel{nullptr};
    QComboBox *m_directionCombo{nullptr};
    QPushButton *m_applyBtn{nullptr};

    // Profiles & Hardware Macros Tab
    QComboBox *m_profileCombo{nullptr};
    QPushButton *m_applyProfileBtn{nullptr};
    QComboBox *m_macroKeyCombo{nullptr};
    QLineEdit *m_macroInput{nullptr};
    QPushButton *m_assignMacroBtn{nullptr};

    // Home Assistant / Daemon Tab
    QLineEdit *m_mqttHostInput{nullptr};
    QSpinBox *m_mqttPortInput{nullptr};
    QLineEdit *m_mqttUserInput{nullptr};
    QLineEdit *m_mqttPassInput{nullptr};
    QPushButton *m_mqttConnectBtn{nullptr};
    QLabel *m_mqttStatusLabel{nullptr};
    QLabel *m_kdeLedStatusLabel{nullptr};

    QLabel *m_msgLabel{nullptr};
    QTimer *m_pollTimer{nullptr};
};

} // namespace thor
