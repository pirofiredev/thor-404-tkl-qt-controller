#include "interactivekeyboardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPainterPath>
#include <algorithm>

namespace thor {

InteractiveKeyboardWidget::InteractiveKeyboardWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumSize(640, 230);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMouseTracking(true);
    initIso88Layout();
}

void InteractiveKeyboardWidget::setMode(Mode mode) {
    m_mode = mode;
    update();
}

void InteractiveKeyboardWidget::setCurrentBrushColor(const QColor &color) {
    m_brushColor = color;
}

void InteractiveKeyboardWidget::setAllKeysColor(const QColor &color) {
    for (auto &k : m_keys) {
        k.color = color;
    }
    update();
    emit keyColorsChanged();
}

void InteractiveKeyboardWidget::setRainbowEffect() {
    for (auto &k : m_keys) {
        // Compute hue across keyboard width (0.0 to 18.25)
        double ratio = std::clamp(k.rect.x() / 18.25, 0.0, 1.0);
        int hue = static_cast<int>(ratio * 359.0);
        k.color = QColor::fromHsv(hue, 240, 255);
    }
    update();
    emit keyColorsChanged();
}

void InteractiveKeyboardWidget::setKeyColor(const std::string &keyId, const QColor &color) {
    for (auto &k : m_keys) {
        if (k.id == keyId) {
            k.color = color;
            break;
        }
    }
    update();
    emit keyColorsChanged();
}

std::map<std::string, QColor> InteractiveKeyboardWidget::getKeyColors() const {
    std::map<std::string, QColor> map;
    for (const auto &k : m_keys) {
        map[k.id] = k.color;
    }
    return map;
}

void InteractiveKeyboardWidget::setSelectedKey(const std::string &keyId) {
    m_lastSelectedKeyId = keyId;
    for (auto &k : m_keys) {
        k.isSelected = (k.id == keyId);
    }
    update();
}

std::string InteractiveKeyboardWidget::getSelectedKeyId() const {
    return m_lastSelectedKeyId;
}

void InteractiveKeyboardWidget::initIso88Layout() {
    m_keys.clear();

    // Key definitions for ISO-88 layout in normalized grid units (18.25 wide, 6.25 high)
    // Row 0: Function row
    auto addKey = [this](const std::string &id, const QString &label,
                         double x, double y, double w, double h,
                         uint8_t line, uint8_t dword, uint8_t scancode) {
        KeyVisual kv;
        kv.id = id;
        kv.label = label;
        kv.rect = QRectF(x, y, w, h);
        kv.line = line;
        kv.dword = dword;
        kv.scancode = scancode;
        kv.color = QColor(255, 0, 0); // default red
        m_keys.push_back(kv);
    };

    // Row 0 (Esc, F1-F12, PrtSc, ScrLk, Pause)
    addKey("escape", "Esc", 0.0, 0.0, 1.0, 1.0, 12, 1, 0x29);
    addKey("f1", "F1", 2.0, 0.0, 1.0, 1.0, 12, 2, 0x3A);
    addKey("f2", "F2", 3.0, 0.0, 1.0, 1.0, 12, 3, 0x3B);
    addKey("f3", "F3", 4.0, 0.0, 1.0, 1.0, 12, 4, 0x3C);
    addKey("f4", "F4", 5.0, 0.0, 1.0, 1.0, 12, 5, 0x3D);
    addKey("f5", "F5", 6.5, 0.0, 1.0, 1.0, 12, 6, 0x3E);
    addKey("f6", "F6", 7.5, 0.0, 1.0, 1.0, 12, 7, 0x3F);
    addKey("f7", "F7", 8.5, 0.0, 1.0, 1.0, 12, 8, 0x40);
    addKey("f8", "F8", 9.5, 0.0, 1.0, 1.0, 12, 9, 0x41);
    addKey("f9", "F9", 11.0, 0.0, 1.0, 1.0, 12, 10, 0x42);
    addKey("f10", "F10", 12.0, 0.0, 1.0, 1.0, 12, 11, 0x43);
    addKey("f11", "F11", 13.0, 0.0, 1.0, 1.0, 12, 12, 0x44);
    addKey("f12", "F12", 14.0, 0.0, 1.0, 1.0, 12, 13, 0x45);
    addKey("prtsc", "Prt", 15.25, 0.0, 1.0, 1.0, 19, 0, 0x46);
    addKey("scrllk", "Scr", 16.25, 0.0, 1.0, 1.0, 19, 1, 0x47);
    addKey("pause", "Pau", 17.25, 0.0, 1.0, 1.0, 19, 3, 0x48);

    // Row 1 (Number row)
    double r1 = 1.25;
    addKey("backtick", "`", 0.0, r1, 1.0, 1.0, 13, 3, 0x35);
    addKey("1", "1", 1.0, r1, 1.0, 1.0, 13, 4, 0x1E);
    addKey("2", "2", 2.0, r1, 1.0, 1.0, 13, 5, 0x1F);
    addKey("3", "3", 3.0, r1, 1.0, 1.0, 13, 6, 0x20);
    addKey("4", "4", 4.0, r1, 1.0, 1.0, 13, 7, 0x21);
    addKey("5", "5", 5.0, r1, 1.0, 1.0, 13, 8, 0x22);
    addKey("6", "6", 6.0, r1, 1.0, 1.0, 13, 9, 0x23);
    addKey("7", "7", 7.0, r1, 1.0, 1.0, 13, 10, 0x24);
    addKey("8", "8", 8.0, r1, 1.0, 1.0, 13, 11, 0x25);
    addKey("9", "9", 9.0, r1, 1.0, 1.0, 13, 12, 0x26);
    addKey("0", "0", 10.0, r1, 1.0, 1.0, 13, 13, 0x27);
    addKey("-", "-", 11.0, r1, 1.0, 1.0, 13, 14, 0x2D);
    addKey("=", "=", 12.0, r1, 1.0, 1.0, 13, 15, 0x2E);
    addKey("backspace", "Bksp", 13.0, r1, 2.0, 1.0, 18, 7, 0x2A);
    addKey("insert", "Ins", 15.25, r1, 1.0, 1.0, 19, 4, 0x49);
    addKey("home", "Hom", 16.25, r1, 1.0, 1.0, 19, 5, 0x4A);
    addKey("page up", "PgUp", 17.25, r1, 1.0, 1.0, 19, 6, 0x4B);

    // Row 2 (Tab, QWERTY...)
    double r2 = 2.30;
    addKey("tab", "Tab", 0.0, r2, 1.5, 1.0, 14, 5, 0x2B);
    addKey("q", "Q", 1.5, r2, 1.0, 1.0, 14, 6, 0x14);
    addKey("w", "W", 2.5, r2, 1.0, 1.0, 14, 7, 0x1A);
    addKey("e", "E", 3.5, r2, 1.0, 1.0, 14, 8, 0x08);
    addKey("r", "R", 4.5, r2, 1.0, 1.0, 14, 9, 0x15);
    addKey("t", "T", 5.5, r2, 1.0, 1.0, 14, 10, 0x17);
    addKey("y", "Y", 6.5, r2, 1.0, 1.0, 14, 11, 0x1C);
    addKey("u", "U", 7.5, r2, 1.0, 1.0, 14, 12, 0x18);
    addKey("i", "I", 8.5, r2, 1.0, 1.0, 14, 13, 0x0C);
    addKey("o", "O", 9.5, r2, 1.0, 1.0, 14, 14, 0x12);
    addKey("p", "P", 10.5, r2, 1.0, 1.0, 14, 15, 0x13);
    addKey("[", "[", 11.5, r2, 1.0, 1.0, 15, 0, 0x2F);
    addKey("]", "]", 12.5, r2, 1.0, 1.0, 15, 1, 0x30);
    addKey("del", "Del", 15.25, r2, 1.0, 1.0, 19, 7, 0x4C);
    addKey("end", "End", 16.25, r2, 1.0, 1.0, 19, 8, 0x4D);
    addKey("page down", "PgDn", 17.25, r2, 1.0, 1.0, 19, 9, 0x4E);

    // Row 3 (Caps, ASDF...)
    double r3 = 3.35;
    addKey("caps lock", "Caps", 0.0, r3, 1.75, 1.0, 15, 7, 0x39);
    addKey("a", "A", 1.75, r3, 1.0, 1.0, 15, 8, 0x04);
    addKey("s", "S", 2.75, r3, 1.0, 1.0, 15, 9, 0x16);
    addKey("d", "D", 3.75, r3, 1.0, 1.0, 15, 10, 0x07);
    addKey("f", "F", 4.75, r3, 1.0, 1.0, 15, 11, 0x09);
    addKey("g", "G", 5.75, r3, 1.0, 1.0, 15, 12, 0x0A);
    addKey("h", "H", 6.75, r3, 1.0, 1.0, 15, 13, 0x0B);
    addKey("j", "J", 7.75, r3, 1.0, 1.0, 15, 14, 0x0D);
    addKey("k", "K", 8.75, r3, 1.0, 1.0, 15, 15, 0x0E);
    addKey("l", "L", 9.75, r3, 1.0, 1.0, 16, 0, 0x0F);
    addKey(";", ";", 10.75, r3, 1.0, 1.0, 16, 1, 0x33);
    addKey("'", "'", 11.75, r3, 1.0, 1.0, 16, 2, 0x34);
    addKey("reverse slash", "#", 12.75, r3, 1.0, 1.0, 16, 3, 0x31);
    // ISO Return key (tall)
    addKey("enter", "Enter", 13.75, r2, 1.25, 2.05, 17, 5, 0x28);

    // Row 4 (Left Shift, ZXCV...)
    double r4 = 4.40;
    addKey("left shift", "Shift", 0.0, r4, 1.25, 1.0, 16, 9, 0xE1);
    addKey("iso_bslash", "\\", 1.25, r4, 1.0, 1.0, 16, 3, 0x64);
    addKey("z", "Z", 2.25, r4, 1.0, 1.0, 16, 10, 0x1D);
    addKey("x", "X", 3.25, r4, 1.0, 1.0, 16, 11, 0x1B);
    addKey("c", "C", 4.25, r4, 1.0, 1.0, 16, 12, 0x06);
    addKey("v", "V", 5.25, r4, 1.0, 1.0, 16, 13, 0x19);
    addKey("b", "B", 6.25, r4, 1.0, 1.0, 16, 14, 0x05);
    addKey("n", "N", 7.25, r4, 1.0, 1.0, 16, 15, 0x11);
    addKey("m", "M", 8.25, r4, 1.0, 1.0, 17, 0, 0x10);
    addKey(",", ",", 9.25, r4, 1.0, 1.0, 17, 1, 0x36);
    addKey("period", ".", 10.25, r4, 1.0, 1.0, 17, 2, 0x37);
    addKey("slash", "/", 11.25, r4, 1.0, 1.0, 17, 3, 0x38);
    addKey("right shift", "Shift", 12.25, r4, 2.75, 1.0, 17, 4, 0xE5);
    addKey("up arrow", "▲", 16.25, r4, 1.0, 1.0, 18, 5, 0x52);

    // Row 5 (Bottom row: Ctrl, Win, Alt, Space...)
    double r5 = 5.45;
    addKey("left ctrl", "Ctrl", 0.0, r5, 1.25, 1.0, 17, 11, 0xE0);
    addKey("windows", "Win", 1.25, r5, 1.25, 1.0, 17, 12, 0xE3);
    addKey("left alt", "Alt", 2.50, r5, 1.25, 1.0, 17, 13, 0xE2);
    addKey("space", "Space", 3.75, r5, 6.25, 1.0, 17, 14, 0x2C);
    addKey("right alt", "AltGr", 10.0, r5, 1.25, 1.0, 17, 15, 0xE6);
    addKey("function", "Fn", 11.25, r5, 1.25, 1.0, 18, 0, 0x00);
    addKey("menu", "Menu", 12.50, r5, 1.25, 1.0, 18, 1, 0x65);
    addKey("right ctrl", "Ctrl", 13.75, r5, 1.25, 1.0, 18, 2, 0xE4);
    addKey("left arrow", "◀", 15.25, r5, 1.0, 1.0, 18, 3, 0x50);
    addKey("down arrow", "▼", 16.25, r5, 1.0, 1.0, 18, 4, 0x51);
    addKey("right arrow", "▶", 17.25, r5, 1.0, 1.0, 18, 6, 0x4F);
}

int InteractiveKeyboardWidget::findKeyAt(const QPointF &pos) const {
    double scaleX = (width() - 28.0) / 18.5;
    double scaleY = (height() - 28.0) / 6.6;
    double unit = std::max(12.0, std::min(scaleX, scaleY));
    double offsetX = std::max(14.0, (width() - 18.5 * unit) / 2.0);
    double offsetY = std::max(14.0, (height() - 6.6 * unit) / 2.0);

    for (size_t i = 0; i < m_keys.size(); ++i) {
        QRectF r(offsetX + m_keys[i].rect.x() * unit,
                 offsetY + m_keys[i].rect.y() * unit,
                 m_keys[i].rect.width() * unit - 2.0,
                 m_keys[i].rect.height() * unit - 2.0);
        if (r.contains(pos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void InteractiveKeyboardWidget::applyBrushToKey(int index) {
    if (index >= 0 && index < static_cast<int>(m_keys.size())) {
        m_keys[index].color = m_brushColor;
        update();
        emit keyColorsChanged();
    }
}

void InteractiveKeyboardWidget::mousePressEvent(QMouseEvent *event) {
    if (!m_isInteractive) {
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        int idx = findKeyAt(event->position());
        if (idx >= 0) {
            if (m_mode == Mode::ColorPicker) {
                applyBrushToKey(idx);
            } else {
                setSelectedKey(m_keys[idx].id);
                emit keySelected(m_keys[idx].label, m_keys[idx].scancode);
            }
        }
    }
}

void InteractiveKeyboardWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!m_isInteractive) {
        return;
    }
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        int idx = findKeyAt(event->position());
        if (idx >= 0) {
            if (m_mode == Mode::ColorPicker) {
                applyBrushToKey(idx);
            }
        }
    }
}

void InteractiveKeyboardWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (!m_isInteractive) {
        m_isDragging = false;
        return;
    }
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

void InteractiveKeyboardWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Calculate layout scaling with safe padding
    double scaleX = (width() - 28.0) / 18.5;
    double scaleY = (height() - 28.0) / 6.6;
    double unit = std::max(12.0, std::min(scaleX, scaleY));
    double offsetX = std::max(14.0, (width() - 18.5 * unit) / 2.0);
    double offsetY = std::max(14.0, (height() - 6.6 * unit) / 2.0);

    // Draw keyboard base bezel
    QRectF baseRect(offsetX - 6, offsetY - 6, 18.5 * unit + 12, 6.6 * unit + 12);
    p.setPen(QPen(palette().color(QPalette::Mid), 1.2));
    p.setBrush(palette().color(QPalette::Window));
    p.drawRoundedRect(baseRect, 8, 8);

    // Draw keys
    QFont font = p.font();
    font.setPointSizeF(std::clamp(unit * 0.28, 7.0, 11.0));
    p.setFont(font);

    for (const auto &k : m_keys) {
        QRectF r(offsetX + k.rect.x() * unit,
                 offsetY + k.rect.y() * unit,
                 k.rect.width() * unit - 2.0,
                 k.rect.height() * unit - 2.0);

        // Keycap background
        QColor capColor = palette().color(QPalette::Button);
        p.setPen(k.isSelected ? QPen(palette().color(QPalette::Highlight), 2.2)
                              : QPen(palette().color(QPalette::Mid), 1.0));
        p.setBrush(capColor);
        p.drawRoundedRect(r, 3, 3);

        // Key LED indicator glow / color pill
        QRectF ledRect(r.x() + 4, r.y() + r.height() - 6, r.width() - 8, 3.5);
        p.setPen(Qt::NoPen);
        p.setBrush(k.color);
        p.drawRoundedRect(ledRect, 1.5, 1.5);

        // Key label
        p.setPen(palette().color(QPalette::ButtonText));
        p.drawText(r.adjusted(0, 0, 0, -4), Qt::AlignCenter, k.label);
    }
}

} // namespace thor
