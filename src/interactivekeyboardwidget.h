#pragma once

#include <QWidget>
#include <QColor>
#include <QPoint>
#include <QRectF>
#include <vector>
#include <map>
#include <string>
#include <cstdint>

namespace thor {

struct KeyVisual {
    std::string id;       // Key identifier (e.g. "escape", "q", "enter")
    QString label;        // UI Label text (e.g. "Esc", "Q", "Enter")
    QRectF rect;          // Visual coordinates in unit grid
    uint8_t line{0};      // Matrix row line (12 - 19)
    uint8_t dword{0};     // Matrix column dword (0 - 15)
    uint8_t scancode{0};  // HID scancode for macros
    QColor color{255, 0, 0};
    bool isSelected{false};
};

class InteractiveKeyboardWidget : public QWidget {
    Q_OBJECT

public:
    enum class Mode {
        ColorPicker, // Clicking/dragging paints RGB color
        MacroKeyPick // Clicking selects key for macro assignment
    };

    explicit InteractiveKeyboardWidget(QWidget *parent = nullptr);
    ~InteractiveKeyboardWidget() override = default;

    void setMode(Mode mode);
    void setCurrentBrushColor(const QColor &color);
    void setAllKeysColor(const QColor &color);
    void setKeyColor(const std::string &keyId, const QColor &color);

    std::map<std::string, QColor> getKeyColors() const;
    void setSelectedKey(const std::string &keyId);
    std::string getSelectedKeyId() const;

    QSize sizeHint() const override { return QSize(600, 220); }
    QSize minimumSizeHint() const override { return QSize(480, 180); }

signals:
    void keySelected(const QString &keyName, uint8_t scancode);
    void keyColorsChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void initIso88Layout();
    int findKeyAt(const QPointF &pos) const;
    void applyBrushToKey(int index);

    std::vector<KeyVisual> m_keys;
    Mode m_mode{Mode::ColorPicker};
    QColor m_brushColor{255, 0, 0};
    bool m_isDragging{false};
    std::string m_lastSelectedKeyId;
};

} // namespace thor
