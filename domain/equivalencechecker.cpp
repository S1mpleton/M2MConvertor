#include "domain/equivalencechecker.h"

#include "domain/isomorphismchecker.h"

#include <QHash>
#include <QQueue>

#include <algorithm>

namespace {
QString outputSignature(const QSet<int>& outputs) {
    QList<int> sorted = outputs.values();
    std::sort(sorted.begin(), sorted.end());
    QStringList parts;
    for (int output : sorted) parts.append(QString::number(output));
    return parts.join(QLatin1Char(','));
}

QString stateSignature(const Automaton& automaton, int state,
                       const QVector<int>& partitions) {
    QStringList reactions;
    for (int input = 0; input < automaton.inputs.size(); ++input) {
        const Transition* edge = automaton.transition(state, input);
        if (!edge) {
            reactions.append(QStringLiteral("%1:missing").arg(input));
            continue;
        }
        reactions.append(QStringLiteral("%1:[%2]->%3")
            .arg(input)
            .arg(outputSignature(edge->outputs))
            .arg(partitions.value(edge->to, -1)));
    }
    return reactions.join(QLatin1Char('|'));
}
}

Automaton EquivalenceChecker::reactionModel(const Automaton& source) {
    Automaton result = source;
    result.kind = AutomatonKind::Mealy;
    result.mooreOutputs.clear();
    if (source.kind == AutomatonKind::Moore) {
        for (Transition& edge : result.transitions)
            edge.outputs = source.mooreOutputs.value(edge.to);
    }
    return result;
}

Automaton EquivalenceChecker::minimizedReachable(const Automaton& source) {
    QVector<bool> reachable(source.states.size(), false);
    QVector<int> order;
    QQueue<int> pending;
    reachable[source.initialState] = true;
    pending.enqueue(source.initialState);
    while (!pending.isEmpty()) {
        const int state = pending.dequeue();
        order.append(state);
        for (int input = 0; input < source.inputs.size(); ++input) {
            const Transition* edge = source.transition(state, input);
            if (edge && !reachable[edge->to]) {
                reachable[edge->to] = true;
                pending.enqueue(edge->to);
            }
        }
    }

    QVector<int> partitions(source.states.size(), -1);
    for (int state : order) partitions[state] = 0;
    for (;;) {
        QHash<QString, int> partitionBySignature;
        QVector<int> refined(source.states.size(), -1);
        for (int state : order) {
            const QString signature = stateSignature(source, state, partitions);
            auto found = partitionBySignature.constFind(signature);
            if (found == partitionBySignature.cend()) {
                const int next = partitionBySignature.size();
                partitionBySignature.insert(signature, next);
                refined[state] = next;
            } else {
                refined[state] = found.value();
            }
        }
        if (refined == partitions) break;
        partitions = refined;
    }

    int blockCount = 0;
    for (int state : order) blockCount = qMax(blockCount, partitions[state] + 1);
    QVector<int> representative(blockCount, -1);
    for (int state : order) {
        const int block = partitions[state];
        if (representative[block] < 0) representative[block] = state;
    }

    Automaton result;
    result.kind = AutomatonKind::Mealy;
    result.inputs = source.inputs;
    result.outputs = source.outputs;
    result.initialState = partitions[source.initialState];
    for (int block = 0; block < blockCount; ++block)
        result.states.append(QStringLiteral("m%1").arg(block));
    for (int block = 0; block < blockCount; ++block) {
        const int state = representative[block];
        for (int input = 0; input < source.inputs.size(); ++input) {
            const Transition* edge = source.transition(state, input);
            if (!edge) continue;
            result.transitions.append(
                {block, input, partitions[edge->to], edge->outputs});
        }
    }
    return result;
}

EquivalenceResult EquivalenceChecker::compare(const Automaton& first,
                                              const Automaton& second) {
    QString error;
    if (!first.validate(&error))
        return {false, QStringLiteral("Эталонный автомат некорректен: %1").arg(error)};
    if (!second.validate(&error))
        return {false, QStringLiteral("Введённый автомат некорректен: %1").arg(error)};
    if (first.inputs.size() != second.inputs.size())
        return {false, QStringLiteral("Не совпадает число входных сигналов.")};
    if (first.outputs.size() != second.outputs.size())
        return {false, QStringLiteral("Не совпадает число выходных сигналов.")};

    const Automaton firstMinimal = minimizedReachable(reactionModel(first));
    const Automaton secondMinimal = minimizedReachable(reactionModel(second));
    if (firstMinimal.states.size() != secondMinimal.states.size()) {
        return {false, QStringLiteral(
            "Автоматы неэквивалентны: после удаления недостижимых состояний и минимизации "
            "получено разное число различимых состояний (%1 и %2).")
            .arg(firstMinimal.states.size()).arg(secondMinimal.states.size())};
    }
    const IsomorphismResult minimalCheck =
        IsomorphismChecker::compare(firstMinimal, secondMinimal);
    if (!minimalCheck.equivalent) {
        return {false, QStringLiteral(
            "Автоматы неэквивалентны: их реакции различаются хотя бы для одной "
            "входной последовательности.")};
    }
    return {true, QStringLiteral(
        "Верно. Автоматы поведенчески эквивалентны для всех входных последовательностей.")};
}
