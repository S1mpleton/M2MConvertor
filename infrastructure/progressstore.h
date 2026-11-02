#ifndef PROGRESSSTORE_H
#define PROGRESSSTORE_H

#include "domain/result.h"
#include <QJsonObject>

class ProgressStore {
  public:
    static Result write(const QString& path, const QJsonObject& data);
    static Result read(const QString& path, QJsonObject* data);
    static bool save(const QString& path, const QJsonObject& data, QString* error = nullptr);
    static bool load(const QString& path, QJsonObject* data, QString* error = nullptr);
};

#endif
