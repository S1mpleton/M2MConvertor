#pragma once
#include "domain/result.h"
#include <QList>
#include <QObject>

class Logger : public QObject {
    Q_OBJECT
  public:
    static Logger& instance();
    void log(const Result& result);
    const QList<Result>& history() const {
        return m_history;
    }
    void clear();
    static QString line(const Result& result);
  signals:
    void appended(const Result& result);
    void cleared();

  private:
    Logger() = default;
    QList<Result> m_history;
};
