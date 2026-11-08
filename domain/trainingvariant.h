#pragma once

#include "domain/automaton.h"
#include <QByteArray>
#include <QVector>

enum class ConversionDirection { MealyToMoore, MooreToMealy };

struct AutomatonCounts {
    int states = 1;
    int inputs = 1;
    int outputs = 1;
    int transitions = 1;
};

struct ReactionStep {
    // An empty input is allowed only for the final output of a Moore reaction.
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
        return direction == ConversionDirection::MooreToMealy || direction == ConversionDirection::MealyToMoore;
    }
    AutomatonKind sourceKind() const {
        return direction == ConversionDirection::MooreToMealy ? AutomatonKind::Moore : AutomatonKind::Mealy;
    }
    AutomatonKind targetKind() const {
        return sourceKind() == AutomatonKind::Moore ? AutomatonKind::Mealy : AutomatonKind::Moore;
    }
};

QString conversionDirectionName(ConversionDirection direction);
