#include "infrastructure/progressstore.h"

#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

bool ProgressStore::save(const QString& path, const QJsonObject& data,
                         QString* error) {
    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("M2MConvertorProgress"));
    root.insert(QStringLiteral("version"), 3);
    root.insert(QStringLiteral("data"), data);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool ProgressStore::load(const QString& path, QJsonObject* data,
                         QString* error) {
    if (!data) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("Файл прогресса повреждён.");
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() !=
            QStringLiteral("M2MConvertorProgress") ||
        (root.value(QStringLiteral("version")).toInt() != 2 &&
         root.value(QStringLiteral("version")).toInt() != 3) ||
        !root.value(QStringLiteral("data")).isObject()) {
        if (error) *error = QStringLiteral("Версия файла прогресса не поддерживается. Работы старого формата с полными автоматами нужно начать заново.");
        return false;
    }
    *data = root.value(QStringLiteral("data")).toObject();
    return true;
}
