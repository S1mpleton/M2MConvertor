#ifndef PROGRESSSTORE_H
#define PROGRESSSTORE_H

#include <QJsonObject>

class ProgressStore {
public:
    static bool save(const QString& path, const QJsonObject& data,
                     QString* error = nullptr);
    static bool load(const QString& path, QJsonObject* data,
                     QString* error = nullptr);
};

#endif
