#pragma once
#include <QScrollArea>

// Use the layout's minimum height at the viewport width. QScrollArea's default
// heightForWidth uses the preferred canvas size, unnecessarily hiding instructions.
class WorkScrollArea : public QScrollArea {
    Q_OBJECT
  public:
    explicit WorkScrollArea(QWidget* content, QWidget* parent = nullptr);
    QSize minimumSizeHint() const override { return QSize(0, 0); }

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

  private:
    void fitContent();
    bool m_fitting = false;
};
