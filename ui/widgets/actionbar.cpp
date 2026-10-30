#include "ui/widgets/actionbar.h"
#include <QLayout>
#include <QEvent>
#include <QVector>

namespace {
class ActionLayout final : public QLayout {
public:
    ActionLayout(QWidget* parent, Qt::Alignment alignment) : QLayout(parent), m_alignment(alignment) {
        setContentsMargins(0, 0, 0, 0);
        setSpacing(8);
    }
    ~ActionLayout() override {
        while (auto* item = takeAt(0)) delete item;
    }
    void addItem(QLayoutItem* item) override { m_items.append(item); invalidate(); }
    int count() const override { return m_items.size(); }
    QLayoutItem* itemAt(int index) const override { return m_items.value(index); }
    QLayoutItem* takeAt(int index) override {
        return index >= 0 && index < m_items.size() ? m_items.takeAt(index) : nullptr;
    }
    Qt::Orientations expandingDirections() const override { return Qt::Horizontal; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override { return arrange(QRect(0, 0, width, 0), false); }
    QSize minimumSize() const override {
        QSize size(0, 0);
        for (auto* item : m_items) size = size.expandedTo(controlSize(item));
        return size;
    }
    QSize sizeHint() const override {
        QSize size(0, 0);
        for (auto* item : m_items) {
            const auto control = controlSize(item);
            size.rwidth() += control.width();
            size.setHeight(qMax(size.height(), control.height()));
        }
        size.rwidth() += qMax(0, count() - 1) * spacing();
        return size;
    }
    void setGeometry(const QRect& rect) override { QLayout::setGeometry(rect); arrange(rect, true); }

private:
    static QSize controlSize(QLayoutItem* item) {
        if (auto* widget = item->widget())
            return widget->sizeHint().expandedTo(widget->minimumSizeHint())
                .expandedTo(widget->minimumSize()).boundedTo(widget->maximumSize());
        return item->sizeHint();
    }
    int arrange(const QRect& rect, bool apply) const {
        int first = 0, y = rect.y();
        while (first < count()) {
            int last = first, rowWidth = 0, rowHeight = 0;
            while (last < count()) {
                const auto size = controlSize(m_items[last]);
                const int nextWidth = rowWidth + (last > first ? spacing() : 0) + size.width();
                if (last > first && nextWidth > rect.width()) break;
                rowWidth = nextWidth;
                rowHeight = qMax(rowHeight, size.height());
                ++last;
            }
            const int extra = qMax(0, rect.width() - rowWidth);
            qreal x = rect.x(), gap = spacing();
            if (m_alignment & Qt::AlignRight) x += extra;
            else if (m_alignment & Qt::AlignHCenter) x += extra / 2.0;
            else if ((m_alignment & Qt::AlignJustify) && last - first > 1)
                gap += qreal(extra) / (last - first - 1);
            if (apply) {
                for (int index = first; index < last; ++index) {
                    const auto size = controlSize(m_items[index]);
                    m_items[index]->setGeometry(QRect(qRound(x), y + (rowHeight - size.height()) / 2,
                                                      size.width(), size.height()));
                    x += size.width() + gap;
                }
            }
            y += rowHeight + (last < count() ? spacing() : 0);
            first = last;
        }
        return y - rect.y();
    }
    QVector<QLayoutItem*> m_items;
    Qt::Alignment m_alignment;
};
}

ActionBar::ActionBar(Qt::Alignment alignment, QWidget* parent) : QWidget(parent) {
    new ActionLayout(this, alignment);
    auto policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}
void ActionBar::addWidget(QWidget* widget) {
    layout()->addWidget(widget);
    updateRequiredHeight();
}
void ActionBar::updateRequiredHeight() {
    if (!layout()) return;
    const int required = layout()->heightForWidth(width());
    if (minimumHeight() != required) setMinimumHeight(required);
}
bool ActionBar::event(QEvent* event) {
    const bool result = QWidget::event(event);
    if (event->type() == QEvent::Resize || event->type() == QEvent::LayoutRequest ||
        event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange)
        updateRequiredHeight();
    return result;
}
