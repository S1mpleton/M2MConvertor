#ifndef VARIANTARCHIVE_H
#define VARIANTARCHIVE_H

#include "domain/automaton.h"

#include <QByteArray>
#include <QList>

enum class ConversionDirection {
    MealyToMoore,
    MooreToMealy
};

struct TrainingVariant {
    int number = 0;
    ConversionDirection direction = ConversionDirection::MealyToMoore;
    Automaton source;
    QByteArray diagram;
    QString diagramFileName;
};

class VariantArchive {
public:
    static bool load(const QString& path, QList<TrainingVariant>* variants,
                     QString* error = nullptr);
};

QString conversionDirectionName(ConversionDirection direction);

#endif
