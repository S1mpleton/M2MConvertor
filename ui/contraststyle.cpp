#include "ui/contraststyle.h"
#include <QApplication>
#include <QPainter>
#include <QStyleOption>
#include <QWidget>

namespace {
QColor outline(const QPalette& palette, QPalette::ColorRole surface) {
    const auto base = palette.color(surface);
    const auto text = palette.color(QPalette::WindowText);
    return QColor::fromRgbF(base.redF() * 0.68 + text.redF() * 0.32,
                            base.greenF() * 0.68 + text.greenF() * 0.32,
                            base.blueF() * 0.68 + text.blueF() * 0.32);
}
void drawOutline(QPainter* painter, const QStyleOption* option, QPalette::ColorRole role) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(outline(option->palette, role), 1));
    painter->drawRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), 3, 3);
    painter->restore();
}
}

int ContrastStyle::styleHint(StyleHint hint, const QStyleOption* option,
                           const QWidget* widget, QStyleHintReturn* data) const {
    if (hint == SH_Table_GridLineColor)
        return int(outline(option ? option->palette : widget ? widget->palette() : QApplication::palette(), QPalette::Base).rgb());
    return QProxyStyle::styleHint(hint, option, widget, data);
}
void ContrastStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
                                  QPainter* painter, const QWidget* widget) const {
    QProxyStyle::drawPrimitive(element, option, painter, widget);
    if (element == PE_PanelButtonCommand)
        drawOutline(painter, option, QPalette::Button);
    else if (element == PE_FrameTabWidget)
        drawOutline(painter, option, QPalette::Window);
}
void ContrastStyle::drawControl(ControlElement element, const QStyleOption* option,
                                QPainter* painter, const QWidget* widget) const {
    QProxyStyle::drawControl(element, option, painter, widget);
    if (element == CE_TabBarTabShape)
        drawOutline(painter, option, QPalette::Window);
}
