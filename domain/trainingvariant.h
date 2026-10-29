#pragma once

#include "domain/automaton.h"
#include <QByteArray>
#include <QVector>

enum class ConversionDirection { MealyToMoore, MooreToMealy };

struct AutomatonCounts {
    int states = 1;
    int inputs = 1;
    int outputs = 1;
};

struct ReactionStep {
    QString input;
    QStringList outputs;
};

// Sequence order, including the stored order of outputs, is retained on import.
struct TrainingVariant {
    int number = 0;
    ConversionDirection direction = ConversionDirection::MooreToMealy;
    AutomatonCounts mealy;
    AutomatonCounts moore;
    QVector<ReactionStep> reaction;
    QByteArray diagram;
    QString diagramFileName;

    const AutomatonCounts& counts(AutomatonKind kind) const {
        return kind == AutomatonKind::Moore ? moore : mealy;
    }
    bool available() const {
        return direction == ConversionDirection::MooreToMealy;
    }
};

QString conversionDirectionName(ConversionDirection direction);
