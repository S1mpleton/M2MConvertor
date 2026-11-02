#pragma once
#include "domain/trainingvariant.h"
#include <QList>

class VariantArchive {
  public:
    static Result read(const QString& path, QList<TrainingVariant>* variants);
    static bool load(const QString& path, QList<TrainingVariant>* variants,
                     QString* error = nullptr);
};
