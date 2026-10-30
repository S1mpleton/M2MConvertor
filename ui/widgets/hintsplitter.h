#pragma once
#include <QSplitter>

// Native splitter mechanics provide a minimum hint size followed by full collapse.
// Even when collapsed the narrow handle stays available to reopen the hint.
class HintSplitter : public QSplitter {
    Q_OBJECT
  public:
    HintSplitter(const char* title, QWidget* hint, QWidget* work, const QString& name,
                 Qt::Orientation orientation = Qt::Vertical, QWidget* parent = nullptr);
    bool expanded() const;
    void setExpanded(bool expanded);
    int hintExtent() const;
    void setHintExtent(int extent);
    void toggleHint();

  protected:
    QSplitterHandle* createHandle() override;

  private:
    int m_lastExtent = 160;
};
