#include "infrastructure/progressstore.h"
#include "domain/result.h"
#include <QFileInfo>

#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

Result ProgressStore::write(const QString& path, const QJsonObject& data) {
    Message message;
    auto* error = &message;
    auto operation = [&]() -> bool {
        QJsonObject root;
        root.insert(QStringLiteral("format"), QStringLiteral("M2MConvertorProgress"));
        root.insert(QStringLiteral("version"), 3);
        root.insert(QStringLiteral("data"), data);
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP("Result", "Ошибка файла: %1."))
                             .arg(file.errorString());
            return false;
        }
        const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
        if (file.write(bytes) != bytes.size() || !file.commit()) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP("Result", "Ошибка файла: %1."))
                             .arg(file.errorString());
            return false;
        }
        return true;
    };
    if (operation())
        return Result::success(Message(QT_TRANSLATE_NOOP("Result", "Работа сохранена: %1."))
                                   .arg(QFileInfo(path).fileName()),
                               ResultCategory::Progress);
    return Result::error(message.isEmpty()
                             ? Message(QT_TRANSLATE_NOOP("Result", "Не удалось прочитать работу."))
                             : message,
                         ResultCategory::Progress)
        .withDetails(path);
}
bool ProgressStore::save(const QString& path, const QJsonObject& data, QString* error) {
    const auto result = write(path, data);
    if (error && !result.ok())
        *error = result.text();
    return result.ok();
}

Result ProgressStore::read(const QString& path, QJsonObject* data) {
    Message message;
    auto* error = &message;
    auto operation = [&]() -> bool {
        if (!data)
            return false;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP("Result", "Ошибка файла: %1."))
                             .arg(file.errorString());
            return false;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP("Result", "Файл прогресса повреждён."));
            return false;
        }
        const QJsonObject root = document.object();
        if (root.value(QStringLiteral("format")).toString() !=
                QStringLiteral("M2MConvertorProgress") ||
            (root.value(QStringLiteral("version")).toInt() != 2 &&
             root.value(QStringLiteral("version")).toInt() != 3) ||
            !root.value(QStringLiteral("data")).isObject()) {
            if (error)
                *error = Message(QT_TRANSLATE_NOOP(
                    "Result", "Версия файла прогресса не поддерживается. Работы старого формата с "
                              "полными автоматами нужно начать заново."));
            return false;
        }
        *data = root.value(QStringLiteral("data")).toObject();
        return true;
    };
    if (operation())
        return Result::success(Message(QT_TRANSLATE_NOOP("Result", "Работа открыта: %1."))
                                   .arg(QFileInfo(path).fileName()),
                               ResultCategory::Progress);
    return Result::error(message.isEmpty()
                             ? Message(QT_TRANSLATE_NOOP("Result", "Не удалось прочитать работу."))
                             : message,
                         ResultCategory::Progress)
        .withDetails(path);
}
bool ProgressStore::load(const QString& path, QJsonObject* data, QString* error) {
    const auto result = read(path, data);
    if (error && !result.ok())
        *error = result.text();
    return result.ok();
}
