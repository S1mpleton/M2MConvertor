#pragma once
#include <QWidget>

// Fixed controls outside scrolling content; wrap whole controls on narrow windows.
class ActionBar : public QWidget {
    Q_OBJECT
public:
    explicit ActionBar(Qt::Alignment alignment = Qt::AlignRight, QWidget* parent = nullptr);
    void addWidget(QWidget* widget);

protected:
    bool event(QEvent* event) override;

private:
    void updateRequiredHeight();
};
