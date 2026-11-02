#include "ui/widgets/actionbar.h"
#include <QHBoxLayout>

ActionBar::ActionBar(Qt::Alignment alignment, QWidget* parent)
    : QWidget(parent), m_alignment(alignment) {
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(8);
    row->setSizeConstraint(QLayout::SetMinimumSize);
    if (alignment & (Qt::AlignRight | Qt::AlignHCenter)) row->addStretch();
    if (alignment & (Qt::AlignLeft | Qt::AlignHCenter)) row->addStretch();
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ActionBar::addWidget(QWidget* widget) {
    auto* row = static_cast<QHBoxLayout*>(layout());
    if ((m_alignment & Qt::AlignJustify) && m_widgetCount > 0) row->addStretch();
    const bool trailingStretch = m_alignment & (Qt::AlignLeft | Qt::AlignHCenter);
    row->insertWidget(row->count() - (trailingStretch ? 1 : 0), widget, 0, Qt::AlignVCenter);
    ++m_widgetCount;
}
