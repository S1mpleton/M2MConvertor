#pragma once
#include "domain/message.h"
#include <QDateTime>
#include <QMetaType>

enum class ResultLevel { Info, Warning, Error };
enum class ResultCategory { Application, Archive, Progress, Validation, Report, Translation };
class Result {
  public:
    static Result success(const Message& message = {},
                          ResultCategory category = ResultCategory::Application) {
        return Result(ResultLevel::Info, category, message);
    }
    static Result warning(const Message& message,
                          ResultCategory category = ResultCategory::Application) {
        return Result(ResultLevel::Warning, category, message);
    }
    static Result error(const Message& message,
                        ResultCategory category = ResultCategory::Application) {
        return Result(ResultLevel::Error, category, message);
    }
    bool ok() const {
        return m_level != ResultLevel::Error;
    }
    ResultLevel level() const {
        return m_level;
    }
    ResultCategory category() const {
        return m_category;
    }
    QString text() const {
        return m_context.isEmpty() ? m_message.text() : m_context.text() + ": " + m_message.text();
    }
    const Message& message() const {
        return m_message;
    }
    const QDateTime& timestamp() const {
        return m_timestamp;
    }
    const QString& details() const {
        return m_details;
    }
    Result withContext(const Message& context) const {
        auto copy = *this;
        copy.m_context = context;
        return copy;
    }
    Result withDetails(const QString& details) const {
        auto copy = *this;
        copy.m_details = details;
        return copy;
    }

  private:
    Result(ResultLevel level, ResultCategory category, const Message& message)
        : m_level(level), m_category(category), m_message(message),
          m_timestamp(QDateTime::currentDateTime()) {}
    ResultLevel m_level;
    ResultCategory m_category;
    Message m_message;
    Message m_context;
    QDateTime m_timestamp;
    QString m_details;
};
Q_DECLARE_METATYPE(Result)
QString resultLevelName(ResultLevel level);
QString resultCategoryName(ResultCategory category);
