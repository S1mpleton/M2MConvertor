#pragma once
#include "domain/trainingvariant.h"
#include <QList>

class VariantArchive {
  public:
    static bool load(const QString& path, QList<TrainingVariant>* variants,
                     QString* error = nullptr);
};
