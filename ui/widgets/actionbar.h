#pragma once
#include <QWidget>

// Fixed controls outside scrolling content. Their full widths set the minimum
// available width, so shrinking the window never rearranges an action row.
class ActionBar : public QWidget {
    Q_OBJECT
public:
    explicit ActionBar(Qt::Alignment alignment = Qt::AlignRight, QWidget* parent = nullptr);
    void addWidget(QWidget* widget);

private:
    Qt::Alignment m_alignment;
    int m_widgetCount = 0;
};
