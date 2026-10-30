#include "ui/uitext.h"
#include "ui/widgets/hintsplitter.h"
#include <QGroupBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace {
class HintHandle : public QSplitterHandle {
  public:
    HintHandle(Qt::Orientation orientation, HintSplitter* splitter)
        : QSplitterHandle(orientation, splitter), m_owner(splitter) {
        setFocusPolicy(Qt::StrongFocus);
        setAutoFillBackground(false);
        UiText::bind(this, "accessibleName", QT_TRANSLATE_NOOP("App", "Размер подсказки"));
        UiText::bind(this, "toolTip", QT_TRANSLATE_NOOP("App", "Потяните границу, чтобы изменить размер подсказки. "
                      "Продолжите движение за упор, чтобы скрыть её. "
                      "Потяните обратно или дважды щёлкните, чтобы раскрыть."));
    }

  protected:
    void paintEvent(QPaintEvent*) override {
        // Do not paint the native handle: some themes fill its entire hit area.
        // The parent page remains visible on both sides of this one-pixel track.
        QPainter painter(this);
        const auto c = rect().center();
        auto track = palette().color(QPalette::WindowText);
        track.setAlpha(80);
        painter.setPen(track);
        if (orientation() == Qt::Vertical)
            painter.drawLine(0, c.y(), width() - 1, c.y());
        else
            painter.drawLine(c.x(), 0, c.x(), height() - 1);
        painter.setPen(palette().color(hasFocus() ? QPalette::Highlight : QPalette::WindowText));
        for (int offset : {-2, 1}) {
            if (orientation() == Qt::Vertical)
                painter.drawLine(c.x() - 18, c.y() + offset, c.x() + 18, c.y() + offset);
            else
                painter.drawLine(c.x() + offset, c.y() - 18, c.x() + offset, c.y() + 18);
        }
    }
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_owner->toggleHint();
            event->accept();
        } else
            QSplitterHandle::mouseDoubleClickEvent(event);
    }
    void keyPressEvent(QKeyEvent* event) override {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            m_owner->toggleHint();
            event->accept();
        } else
            QSplitterHandle::keyPressEvent(event);
    }

  private:
    HintSplitter* m_owner;
};
} // namespace
HintSplitter::HintSplitter(const char* title, QWidget* hint, QWidget* work, const QString& name,
                           Qt::Orientation orientation, QWidget* parent)
    : QSplitter(orientation, parent) {
    setObjectName(name);
    setContentsMargins(0, 0, 0, 0);
    setHandleWidth(20);
    setChildrenCollapsible(false);
    auto* group = UiText::make<QGroupBox>(title);
    group->setObjectName(name + "Content");
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(0);
    layout->addWidget(hint);
    if (orientation == Qt::Vertical)
        group->setMinimumHeight(fontMetrics().height() + 88);
    else
        group->setMinimumWidth(140);
    addWidget(group);
    addWidget(work);
    setCollapsible(0, true);
    setCollapsible(1, false);
    setStretchFactor(0, 0);
    setStretchFactor(1, 1);
    m_lastExtent = orientation == Qt::Vertical ? 200 : 340;
    setHintExtent(m_lastExtent);
    connect(this, &QSplitter::splitterMoved, this, [this] {
        if (expanded())
            m_lastExtent = hintExtent();
    });
}
QSplitterHandle* HintSplitter::createHandle() {
    return new HintHandle(orientation(), this);
}
bool HintSplitter::expanded() const {
    return hintExtent() > 0;
}
int HintSplitter::hintExtent() const {
    return sizes().value(0);
}
void HintSplitter::setHintExtent(int extent) {
    const int total = orientation() == Qt::Vertical ? height() : width();
    setSizes({qMax(0, extent), qMax(80, total - handleWidth() - extent)});
    if (extent > 0)
        m_lastExtent = extent;
}
void HintSplitter::setExpanded(bool expanded) {
    setHintExtent(expanded ? m_lastExtent : 0);
}
void HintSplitter::toggleHint() {
    setExpanded(!expanded());
}
