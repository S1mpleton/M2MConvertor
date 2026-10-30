#pragma once
#include <QCoreApplication>
#include <QString>
#include <QVariant>
#include <QVector>

// Retain source text and arguments so history can be translated after a language switch.
class Message {
  public:
    Message() = default;
    Message(const char* source) : m_source(QString::fromUtf8(source)) {}
    Message(const QString& rawText) : m_source(rawText) {}
    template <typename... Args> Message arg(const Args&... values) const {
        Message copy = *this;
        (copy.m_arguments.append(argument(values)), ...);
        return copy;
    }
    QString text() const {
        auto value =
            m_raw ? m_source : QCoreApplication::translate("Result", m_source.toUtf8().constData());
        for (const auto& argument : m_arguments)
            value = value.arg(argument.text());
        return value;
    }
    bool isEmpty() const {
        return m_source.isEmpty();
    }
    const QString& source() const {
        return m_source;
    }
    const QVector<Message>& arguments() const {
        return m_arguments;
    }

  private:
    QString m_source;
    QVector<Message> m_arguments;
    bool m_raw = false;
    static Message argument(const Message& value) {
        return value;
    }
    template <typename T> static Message argument(const T& value) {
        Message message(QVariant::fromValue(value).toString());
        message.m_raw = true;
        return message;
    }
};
