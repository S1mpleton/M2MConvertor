#pragma once
#include "domain/result.h"
#include <QDialog>
namespace Ui {
class LogWindow;
}
class LogWindow : public QDialog {
    Q_OBJECT
  public:
    explicit LogWindow(QWidget* parent = nullptr);
    ~LogWindow() override;
    Result saveTo(const QString& path) const;
  signals:
    void notification(const Result& result);

  protected:
    void changeEvent(QEvent*) override;

  private:
    void rebuild();
    void append(const Result& result);
    void save();
    bool m_ready = false;
    Ui::LogWindow* ui;
};
