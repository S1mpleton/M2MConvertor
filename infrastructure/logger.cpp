#include "infrastructure/logger.h"
#include <QDebug>
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}
void Logger::log(const Result& result) {
    if (m_history.size() >= 1000)
        m_history.removeFirst();
    m_history.append(result);
    qInfo().noquote() << line(result);
    emit appended(result);
}
void Logger::clear() {
    m_history.clear();
    emit cleared();
}
QString Logger::line(const Result& result) {
    auto line =
        QStringLiteral("[%1] [%2] [%3] %4")
            .arg(result.timestamp().toString(Qt::ISODateWithMs), resultLevelName(result.level()),
                 resultCategoryName(result.category()), result.text());
    if (!result.details().isEmpty())
        line += " | " + result.details();
    return line;
}
