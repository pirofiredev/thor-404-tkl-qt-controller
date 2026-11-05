#pragma once

#include <QMainWindow>
#include <QColor>
#include <memory>
#include "devicemanager.h"

class QLabel;
class QComboBox;
class QPushButton;
class QSlider;
class QCheckBox;
class QTimer;

namespace thor {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onRefreshDevices();
    void onModelSelected(int index);
    void onEffectChanged(int index);
    void onPickColor();
    void onApply();

private:
    void setupUi();
    void updateStatusBadge(bool connected);
    void updateSwatch();

    DeviceManager m_deviceManager;
    QColor m_selectedColor{255, 0, 0}; // default red

    QLabel *m_statusBadge{nullptr};
    QComboBox *m_modelCombo{nullptr};
    QComboBox *m_effectCombo{nullptr};
    QPushButton *m_colorBtn{nullptr};
    QCheckBox *m_multicolorCheck{nullptr};
    QSlider *m_brightnessSlider{nullptr};
    QLabel *m_brightnessValLabel{nullptr};
    QSlider *m_speedSlider{nullptr};
    QLabel *m_speedValLabel{nullptr};
    QComboBox *m_directionCombo{nullptr};
    QPushButton *m_applyBtn{nullptr};
    QLabel *m_msgLabel{nullptr};
    QTimer *m_pollTimer{nullptr};
};

} // namespace thor
