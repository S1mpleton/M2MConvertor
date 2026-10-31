#pragma once
#include <QPushButton>
#include <QStyleOptionButton>
#include <QStylePainter>

// Keep primary actions on one baseline even on narrow screens. The original
// caption remains available to accessibility and through the tooltip.
class ElidingButton : public QPushButton {
public:
    explicit ElidingButton(QWidget* parent = nullptr) : QPushButton(parent) {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    void setFullCaption(bool full) { m_fullCaption = full; updateGeometry(); }
    QSize minimumSizeHint() const override {
        auto hint = QPushButton::minimumSizeHint();
        if (m_fullCaption) return hint;
        hint.setWidth(iconSize().width() + fontMetrics().horizontalAdvance("…") + 24);
        return hint;
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QStyleOptionButton option;
        initStyleOption(&option);
        if (!m_fullCaption) {
            const int decoration = QPushButton::sizeHint().width() - fontMetrics().horizontalAdvance(text());
            option.text = fontMetrics().elidedText(text(), Qt::ElideRight, qMax(0, width() - decoration));
        }
        QStylePainter painter(this);
        painter.drawControl(QStyle::CE_PushButton, option);
    }
private:
    bool m_fullCaption = false;
};
