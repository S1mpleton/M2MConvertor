#include "domain/conversionservice.h"

#include <algorithm>

namespace {
QString outputKey(const QSet<int>& values) {
    QList<int> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    QStringList parts;
    for (int value : sorted) parts.append(QString::number(value));
    return parts.join(QLatin1Char(','));
}

QString pairKey(int state, const QSet<int>& outputs) {
    return QString::number(state) + QLatin1Char('|') + outputKey(outputs);
}
}

bool ConversionService::convert(const Automaton& source, Automaton* target,
                                QString* error) {
    if (!target) return false;
    QString validationError;
    if (!source.validate(&validationError)) {
        if (error) *error = validationError;
        return false;
    }
    return source.kind == AutomatonKind::Mealy
        ? mealyToMoore(source, target, error)
        : mooreToMealy(source, target, error);
}

bool ConversionService::mooreToMealy(const Automaton& source,
                                     Automaton* target, QString*) {
    Automaton result;
    result.kind = AutomatonKind::Mealy;
    result.states = source.states;
    result.inputs = source.inputs;
    result.outputs = source.outputs;
    result.initialState = source.initialState;
    for (const Transition& sourceEdge : source.transitions) {
        Transition edge = sourceEdge;
        edge.outputs = source.mooreOutputs.value(edge.to);
        result.transitions.append(edge);
    }
    *target = result;
    return true;
}

bool ConversionService::mealyToMoore(const Automaton& source,
                                     Automaton* target, QString* error) {
    struct Pair { int state; QSet<int> outputs; };
    QVector<Pair> pairs;
    QHash<QString, int> pairIndexes;
    auto ensurePair = [&](int state, const QSet<int>& outputs) {
        const QString key = pairKey(state, outputs);
        auto found = pairIndexes.constFind(key);
        if (found != pairIndexes.cend()) return found.value();
        const int index = pairs.size();
        pairs.append({state, outputs});
        pairIndexes.insert(key, index);
        return index;
    };

    // The synthetic initial Moore state has no observable output. It preserves
    // the Mealy reaction from the first input even if the initial source state
    // has incoming transitions with several different outputs.
    const int initialPair = ensurePair(source.initialState, {});
    for (const Transition& edge : source.transitions)
        ensurePair(edge.to, edge.outputs);

    Automaton result;
    result.kind = AutomatonKind::Moore;
    result.inputs = source.inputs;
    result.outputs = source.outputs;
    result.initialState = initialPair;
    for (const Pair& pair : pairs) {
        const QString suffix = outputKey(pair.outputs);
        result.states.append(QStringLiteral("%1{%2}")
                                 .arg(source.states[pair.state], suffix));
        result.mooreOutputs.append(pair.outputs);
    }
    for (int pairIndex = 0; pairIndex < pairs.size(); ++pairIndex) {
        const Pair& pair = pairs[pairIndex];
        for (int input = 0; input < source.inputs.size(); ++input) {
            const Transition* sourceEdge = source.transition(pair.state, input);
            if (!sourceEdge) continue;
            const int destination = pairIndexes.value(
                pairKey(sourceEdge->to, sourceEdge->outputs), -1);
            if (destination < 0) {
                if (error) *error = QStringLiteral("Не удалось построить состояние автомата Мура.");
                return false;
            }
            result.transitions.append({pairIndex, input, destination, {}});
        }
    }
    *target = result;
    return true;
}
