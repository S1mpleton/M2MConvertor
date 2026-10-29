#pragma once
#include <QPainter>
#include <QStyleOptionTab>
#include <QStylePainter>
#include <QTabBar>
#include <QTabWidget>

// A cross and strike-through distinguish unavailable steps independently of color/theme.
class WorkflowTabBar : public QTabBar {
  protected:
    void paintEvent(QPaintEvent*) override {
        QStylePainter painter(this);
        for (int i = 0; i < count(); ++i) {
            QStyleOptionTab option;
            initStyleOption(&option, i);
            const bool enabled = isTabEnabled(i);
            painter.setOpacity(enabled ? 1.0 : 0.45);
            QFont f = font();
            f.setStrikeOut(!enabled);
            f.setBold(enabled && currentIndex() == i);
            painter.setFont(f);
            if (!enabled)
                option.text = QString::fromUtf8("× ") + option.text;
            painter.drawControl(QStyle::CE_TabBarTabShape, option);
            painter.drawControl(QStyle::CE_TabBarTabLabel, option);
        }
    }
    QSize tabSizeHint(int index) const override {
        return QTabBar::tabSizeHint(index) + QSize(18, 4);
    }
};
class WorkflowTabs : public QTabWidget {
  public:
    explicit WorkflowTabs(QWidget* parent = nullptr) : QTabWidget(parent) {
        setTabBar(new WorkflowTabBar);
    }
};
