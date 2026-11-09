#include "infrastructure/logger.h"
#include <QDebug>
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}
void Logger::log(const Result& result) {
    const auto entry = result.studentFacing();
    if (m_history.size() >= 1000)
        m_history.removeFirst();
    m_history.append(entry);
    qInfo().noquote() << line(entry);
    emit appended(entry);
}
void Logger::clear() {
    m_history.clear();
    emit cleared();
}
QString Logger::line(const Result& result) {
    const auto entry = result.studentFacing();
    auto line =
        QStringLiteral("[%1] [%2] [%3] %4")
            .arg(entry.timestamp().toString(Qt::ISODateWithMs), resultLevelName(entry.level()),
                 resultCategoryName(entry.category()), entry.text());
    if (!entry.details().isEmpty())
        line += " | " + entry.details();
    return line;
}
