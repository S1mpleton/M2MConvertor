#include "ui/widgets/workscrollarea.h"
#include <QBoxLayout>
#include <QEvent>
#include <QResizeEvent>

WorkScrollArea::WorkScrollArea(QWidget* content, QWidget* parent) : QScrollArea(parent) {
    setFrameShape(QFrame::NoFrame);
    setMinimumSize(0, 0);
    setWidgetResizable(false);
    content->layout()->setSizeConstraint(QLayout::SetDefaultConstraint);
    setWidget(content);
    content->installEventFilter(this);
    viewport()->installEventFilter(this);
    fitContent();
}
void WorkScrollArea::fitContent() {
    if (m_fitting || !widget())
        return;
    m_fitting = true;
    auto* box = qobject_cast<QBoxLayout*>(widget()->layout());
    box->activate();
    const int width = qMax(viewport()->width(), box->minimumSize().width());
    const int minimumHeight = qMax(box->minimumSize().height(), box->minimumHeightForWidth(width));
    widget()->resize(width, qMax(viewport()->height(), minimumHeight));
    m_fitting = false;
}
bool WorkScrollArea::eventFilter(QObject* object, QEvent* event) {
    const bool result = QScrollArea::eventFilter(object, event);
    if ((object == widget() && event->type() == QEvent::LayoutRequest) ||
        (object == viewport() && event->type() == QEvent::Resize))
        fitContent();
    return result;
}
void WorkScrollArea::resizeEvent(QResizeEvent* event) {
    QScrollArea::resizeEvent(event);
    fitContent();
}
