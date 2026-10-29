#include "domain/reactionchecker.h"
#include <QHash>
#include <QRegularExpression>

QStringList ReactionChecker::stateNames(int count) {
    QStringList names;
    for (int i = 0; i < count; ++i)
        names.append(QStringLiteral("a%1").arg(i));
    return names;
}

ReactionCheckResult ReactionChecker::check(const TrainingVariant& task,
                                           const Automaton& candidate) {
    if (!task.available())
        return {false, QStringLiteral("Преобразование Мили → Мура находится в разработке.")};
    QString error;
    if (!candidate.validate(&error))
        return {false, error};
    const auto& counts = task.counts(candidate.kind);
    if (candidate.states != stateNames(counts.states) || candidate.initialState != 0)
        return {false, QStringLiteral("Нужны состояния a0…a%1; начальное состояние — a0.")
                           .arg(counts.states - 1)};
    if (candidate.inputs.size() != counts.inputs || candidate.outputs.size() != counts.outputs)
        return {false, QStringLiteral("Автомат %1: требуется %2 входных и %3 выходных сигналов; "
                                      "введено %4 и %5.")
                           .arg(automatonKindName(candidate.kind))
                           .arg(counts.inputs)
                           .arg(counts.outputs)
                           .arg(candidate.inputs.size())
                           .arg(candidate.outputs.size())};
    const QRegularExpression inputPattern(QStringLiteral("^x[0-9]$"));
    const QRegularExpression outputPattern(QStringLiteral("^y[0-9]$"));
    for (const auto& input : candidate.inputs)
        if (!inputPattern.match(input).hasMatch())
            return {false, QStringLiteral("Входной сигнал должен иметь вид x0…x9.")};
    for (const auto& output : candidate.outputs)
        if (!outputPattern.match(output).hasMatch())
            return {false, QStringLiteral("Выходной сигнал должен иметь вид y0…y9.")};
    if (task.reaction.isEmpty())
        return {false, QStringLiteral("В варианте отсутствует реакция для проверки.")};

    // Build an index once: checking a long ordered trace is O(transitions + steps).
    QHash<quint64, const Transition*> edges;
    auto key = [](int state, int input) {
        return (quint64(quint32(state)) << 32) | quint32(input);
    };
    for (const auto& edge : candidate.transitions)
        edges.insert(key(edge.from, edge.input), &edge);
    int state = 0;
    for (int i = 0; i < task.reaction.size(); ++i) {
        const auto& expected = task.reaction[i];
        const int input = candidate.inputs.indexOf(expected.input);
        const auto* edge = edges.value(key(state, input), nullptr);
        if (!edge)
            return {false,
                    QStringLiteral("Такт %1: из %2 не задан переход по %3.")
                        .arg(i + 1)
                        .arg(candidate.states[state], expected.input),
                    i};
        // Moore output is observed after consuming the input, at the destination.
        const auto& values = candidate.kind == AutomatonKind::Moore
                                 ? candidate.mooreOutputs[edge->to]
                                 : edge->outputs;
        QSet<QString> actual;
        for (int output : values)
            actual.insert(candidate.outputs[output]);
        const QSet<QString> wanted(expected.outputs.begin(), expected.outputs.end());
        if (actual != wanted) {
            QStringList observed(actual.begin(), actual.end());
            observed.sort();
            return {false,
                    QStringLiteral("Такт %1, %2 из %3: ожидались выходы %4, получены %5.")
                        .arg(i + 1)
                        .arg(expected.input, candidate.states[state],
                             expected.outputs.isEmpty() ? QStringLiteral("—")
                                                        : expected.outputs.join(", "),
                             observed.isEmpty() ? QStringLiteral("—") : observed.join(", ")),
                    i};
        }
        state = edge->to;
    }
    return {true, QStringLiteral("Верно. Реакция из a0 совпадает с вариантом во всех %1 тактах.")
                      .arg(task.reaction.size())};
}
