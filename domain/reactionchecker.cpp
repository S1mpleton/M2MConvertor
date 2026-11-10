#include "domain/reactionchecker.h"
#include <QHash>
#include <QRegularExpression>

QStringList ReactionChecker::stateNames(int count) {
    QStringList names;
    for (int i = 0; i < count; ++i)
        names.append(QStringLiteral("a%1").arg(i));
    return names;
}

ReactionCheckResult ReactionChecker::check(const TrainingVariant& task, const Automaton& candidate,
                                           bool graph) {
    if (!task.available())
        return {false, Message(QT_TRANSLATE_NOOP("Result", "Неизвестный тип преобразования."))};
    const auto valid = candidate.validation();
    if (!valid.ok()) return {false, valid.message()};
    const auto& counts = task.counts(candidate.kind);
    if (candidate.states != stateNames(graph ? candidate.states.size() : counts.states) ||
        (graph && candidate.states.size() > 10) || candidate.initialState != 0)
        return {false, Message(QT_TRANSLATE_NOOP("Result", "Нужны состояния a0…a%1; начальное состояние — a0."))
                           .arg(counts.states - 1)};
    if (candidate.inputs.size() != counts.inputs || candidate.outputs.size() != counts.outputs)
        return {false, Message(QT_TRANSLATE_NOOP("Result", "Автомат %1: требуется %2 входных и %3 выходных сигналов; "
                                      "введено %4 и %5."))
                           .arg(candidate.kind == AutomatonKind::Mealy ? Message(QT_TRANSLATE_NOOP("Result", "Мили")) : Message(QT_TRANSLATE_NOOP("Result", "Мура")))
                           .arg(counts.inputs)
                           .arg(counts.outputs)
                           .arg(candidate.inputs.size())
                           .arg(candidate.outputs.size())};
    const QRegularExpression inputPattern(QStringLiteral("^x[0-9]$"));
    const QRegularExpression outputPattern(QStringLiteral("^y[0-9]$"));
    for (const auto& input : candidate.inputs)
        if (!inputPattern.match(input).hasMatch())
            return {false, Message(QT_TRANSLATE_NOOP("Result", "Входной сигнал должен иметь вид x0…x9."))};
    for (const auto& output : candidate.outputs)
        if (!outputPattern.match(output).hasMatch())
            return {false, Message(QT_TRANSLATE_NOOP("Result", "Выходной сигнал должен иметь вид y0…y9."))};
    if (task.reaction.isEmpty())
        return {false, Message(QT_TRANSLATE_NOOP("Result", "В варианте отсутствует реакция для проверки."))};
    const bool terminalMooreOutput = task.direction == ConversionDirection::MooreToMealy &&
        task.reaction.last().input.isEmpty();
    for (int i = 0; i < task.reaction.size(); ++i)
        if (task.reaction[i].input.isEmpty() && !(terminalMooreOutput && i + 1 == task.reaction.size()))
            return {false, Message(QT_TRANSLATE_NOOP("Result", "Такт %1: пустой вход допустим только в последнем такте реакции Мура."))
                               .arg(i + 1), i};

    // A complete Moore trace has N inputs and N+1 state outputs. Its equivalent
    // Mealy trace has N transitions, each emitting the next Moore state's output.
    // Archives without an explicit terminal observation retain their trace convention.
    const bool shiftedMealy = terminalMooreOutput && candidate.kind == AutomatonKind::Mealy;
    const bool destinationMooreOutput = task.direction == ConversionDirection::MealyToMoore &&
        candidate.kind == AutomatonKind::Moore;
    const int stepCount = task.reaction.size() - (shiftedMealy ? 1 : 0);

    // Build an index once: checking a long ordered trace is O(transitions + steps).
    QHash<quint64, const Transition*> edges;
    auto key = [](int state, int input) {
        return (quint64(quint32(state)) << 32) | quint32(input);
    };
    for (const auto& edge : candidate.transitions)
        edges.insert(key(edge.from, edge.input), &edge);
    int state = 0;
    for (int i = 0; i < stepCount; ++i) {
        const auto& signal = task.reaction[i].input;
        const auto& expected = task.reaction[i + (shiftedMealy ? 1 : 0)];
        const bool outputOnly = terminalMooreOutput && !shiftedMealy && i + 1 == stepCount;
        const int input = outputOnly ? -1 : candidate.inputs.indexOf(signal);
        if (input < 0 && !outputOnly)
            return {false,
                    Message(QT_TRANSLATE_NOOP("Result", "Такт %1: не задан вход %2.")).arg(i + 1).arg(signal), i};
        const auto* edge = outputOnly ? nullptr : edges.value(key(state, input), nullptr);
        if (!edge && (candidate.kind == AutomatonKind::Mealy || destinationMooreOutput))
            return {false,
                    Message(QT_TRANSLATE_NOOP("Result", "Такт %1: из %2 не задан переход по %3."))
                        .arg(i + 1)
                        .arg(candidate.states[state], signal),
                    i};
        // For a Mealy source, compare the Moore state reached by this input.
        const auto& values =
            candidate.kind == AutomatonKind::Moore
                ? candidate.mooreOutputs[destinationMooreOutput ? edge->to : state] : edge->outputs;
        QSet<QString> actual;
        for (int output : values)
            actual.insert(candidate.outputs[output]);
        const QSet<QString> wanted(expected.outputs.begin(), expected.outputs.end());
        if (actual != wanted) {
            QStringList observed(actual.begin(), actual.end());
            observed.sort();
            return {false,
                    Message(QT_TRANSLATE_NOOP("Result", "Такт %1, %2 из %3: ожидались выходы %4, получены %5."))
                        .arg(i + 1)
                        .arg(signal.isEmpty() ? QStringLiteral("—") : signal, candidate.states[state],
                             expected.outputs.isEmpty() ? QStringLiteral("—")
                                                        : expected.outputs.join(", "),
                             observed.isEmpty() ? QStringLiteral("—") : observed.join(", ")),
                    i};
        }
        if (i + 1 < stepCount) {
            if (!edge)
                return {false,
                        Message(QT_TRANSLATE_NOOP("Result", "Такт %1: из %2 нет перехода по %3 для продолжения реакции."))
                            .arg(i + 1)
                            .arg(candidate.states[state], signal),
                        i};
            state = edge->to;
        }
    }
    if (candidate.transitions.size() != counts.transitions)
        return {false, Message(QT_TRANSLATE_NOOP("Result", "Количество переходов автомата %1 не соответствует варианту: требуется %2, введено %3."))
            .arg(candidate.kind == AutomatonKind::Moore ? Message(QT_TRANSLATE_NOOP("Result", "Мура")) : Message(QT_TRANSLATE_NOOP("Result", "Мили")))
            .arg(counts.transitions).arg(candidate.transitions.size())};
    return {true, Message(QT_TRANSLATE_NOOP("Result", "Верно. Реакция из a0 совпадает с вариантом во всех %1 тактах."))
                      .arg(stepCount)};
}
