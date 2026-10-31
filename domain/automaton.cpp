#include "domain/result.h"
#include "domain/automaton.h"

#include <QJsonArray>

#include <algorithm>

namespace {
QJsonArray stringsToJson(const QStringList& values) {
    QJsonArray result;
    for (const QString& value : values) result.append(value);
    return result;
}

bool stringsFromJson(const QJsonValue& value, QStringList* result) {
    if (!result || !value.isArray()) return false;
    result->clear();
    for (const QJsonValue& item : value.toArray()) {
        if (!item.isString()) return false;
        result->append(item.toString());
    }
    return true;
}

QJsonArray indexesToJson(const QSet<int>& values) {
    QList<int> sorted(values.begin(), values.end());
    std::sort(sorted.begin(), sorted.end());
    QJsonArray result;
    for (int value : sorted) result.append(value);
    return result;
}

bool indexesFromJson(const QJsonValue& value, QSet<int>* result) {
    if (!result || !value.isArray()) return false;
    result->clear();
    for (const QJsonValue& item : value.toArray()) {
        if (!item.isDouble()) return false;
        result->insert(item.toInt(-1));
    }
    return true;
}

bool namesValid(const QStringList& names) {
    QSet<QString> unique;
    for (const QString& raw : names) {
        const QString name = raw.trimmed();
        if (name.isEmpty() || unique.contains(name)) return false;
        unique.insert(name);
    }
    return true;
}
}

QString automatonKindName(AutomatonKind kind) {
    return kind == AutomatonKind::Mealy ? QCoreApplication::translate("Result", "Мили")
                                         : QCoreApplication::translate("Result", "Мура");
}

const Transition* Automaton::transition(int state, int inputIndex) const {
    for (const Transition& edge : transitions) {
        if (edge.from == state && edge.input == inputIndex) return &edge;
    }
    return nullptr;
}

Result Automaton::validation() const {
    auto fail = [](const Message& message) { return Result::error(message, ResultCategory::Validation); };
    if (states.isEmpty()) return fail(Message(QT_TRANSLATE_NOOP("Result", "Список состояний пуст.")));
    if (inputs.isEmpty()) return fail(Message(QT_TRANSLATE_NOOP("Result", "Список входных сигналов пуст.")));
    if (outputs.isEmpty()) return fail(Message(QT_TRANSLATE_NOOP("Result", "Список выходных сигналов пуст.")));
    if (!namesValid(states) || !namesValid(inputs) || !namesValid(outputs))
        return fail(Message(QT_TRANSLATE_NOOP("Result", "Имена должны быть непустыми и уникальными внутри списка.")));
    if (initialState < 0 || initialState >= states.size())
        return fail(Message(QT_TRANSLATE_NOOP("Result", "Начальное состояние не входит в список состояний.")));
    if (kind == AutomatonKind::Moore && mooreOutputs.size() != states.size())
        return fail(Message(QT_TRANSLATE_NOOP("Result", "Не заданы выходы всех состояний автомата Мура.")));

    QSet<quint64> cells;
    for (const Transition& edge : transitions) {
        if (edge.from < 0 || edge.from >= states.size() ||
            edge.to < 0 || edge.to >= states.size() ||
            edge.input < 0 || edge.input >= inputs.size())
            return fail(Message(QT_TRANSLATE_NOOP("Result", "Переход содержит неизвестное состояние или вход.")));
        const quint64 key = (quint64(quint32(edge.from)) << 32) |
                            quint32(edge.input);
        if (cells.contains(key))
            return fail(Message(QT_TRANSLATE_NOOP("Result", "Для состояния и входа задано несколько переходов.")));
        cells.insert(key);
        for (int output : edge.outputs) {
            if (output < 0 || output >= outputs.size())
                return fail(Message(QT_TRANSLATE_NOOP("Result", "Переход содержит неизвестный выход.")));
        }
        if (kind == AutomatonKind::Moore && !edge.outputs.isEmpty())
            return fail(Message(QT_TRANSLATE_NOOP("Result", "Выход автомата Мура должен относиться к состоянию.")));
    }
    for (const QSet<int>& stateOutputs : mooreOutputs) {
        for (int output : stateOutputs) {
            if (output < 0 || output >= outputs.size())
                return fail(Message(QT_TRANSLATE_NOOP("Result", "Состояние содержит неизвестный выход.")));
        }
    }
    return Result::success();
}

bool Automaton::validate(QString* error) const {
    const auto result = validation(); if (error && !result.ok()) *error = result.text(); return result.ok();
}

QJsonObject Automaton::toJson() const {
    QJsonObject object;
    object.insert(QStringLiteral("kind"),
                  kind == AutomatonKind::Mealy ? QStringLiteral("Mealy")
                                                : QStringLiteral("Moore"));
    object.insert(QStringLiteral("states"), stringsToJson(states));
    object.insert(QStringLiteral("inputs"), stringsToJson(inputs));
    object.insert(QStringLiteral("outputs"), stringsToJson(outputs));
    object.insert(QStringLiteral("initialState"), initialState);
    QJsonArray edgeArray;
    for (const Transition& edge : transitions) {
        QJsonObject item;
        item.insert(QStringLiteral("from"), edge.from);
        item.insert(QStringLiteral("input"), edge.input);
        item.insert(QStringLiteral("to"), edge.to);
        item.insert(QStringLiteral("outputs"), indexesToJson(edge.outputs));
        edgeArray.append(item);
    }
    object.insert(QStringLiteral("transitions"), edgeArray);
    QJsonArray stateOutputArray;
    for (const QSet<int>& values : mooreOutputs)
        stateOutputArray.append(indexesToJson(values));
    object.insert(QStringLiteral("mooreOutputs"), stateOutputArray);
    return object;
}

bool Automaton::fromJson(const QJsonObject& object, Automaton* result,
                         QString* error) {
    if (!result) return false;
    Automaton parsed;
    const QString kindName = object.value(QStringLiteral("kind")).toString();
    if (kindName == QStringLiteral("Mealy")) parsed.kind = AutomatonKind::Mealy;
    else if (kindName == QStringLiteral("Moore")) parsed.kind = AutomatonKind::Moore;
    else {
        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Неизвестный тип автомата.")).text();
        return false;
    }
    if (!stringsFromJson(object.value(QStringLiteral("states")), &parsed.states) ||
        !stringsFromJson(object.value(QStringLiteral("inputs")), &parsed.inputs) ||
        !stringsFromJson(object.value(QStringLiteral("outputs")), &parsed.outputs)) {
        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Повреждены списки имён автомата.")).text();
        return false;
    }
    parsed.initialState = object.value(QStringLiteral("initialState")).toInt(-1);
    const QJsonValue edgeValue = object.value(QStringLiteral("transitions"));
    if (!edgeValue.isArray()) {
        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Повреждён список переходов.")).text();
        return false;
    }
    for (const QJsonValue& value : edgeValue.toArray()) {
        if (!value.isObject()) return false;
        const QJsonObject item = value.toObject();
        Transition edge;
        edge.from = item.value(QStringLiteral("from")).toInt(-1);
        edge.input = item.value(QStringLiteral("input")).toInt(-1);
        edge.to = item.value(QStringLiteral("to")).toInt(-1);
        if (!indexesFromJson(item.value(QStringLiteral("outputs")), &edge.outputs))
            return false;
        parsed.transitions.append(edge);
    }
    const QJsonValue stateOutputValue = object.value(QStringLiteral("mooreOutputs"));
    if (stateOutputValue.isArray()) {
        for (const QJsonValue& value : stateOutputValue.toArray()) {
            QSet<int> values;
            if (!indexesFromJson(value, &values)) return false;
            parsed.mooreOutputs.append(values);
        }
    }
    if (parsed.kind == AutomatonKind::Mealy) parsed.mooreOutputs.clear();
    QString validationError;
    if (!parsed.validate(&validationError)) {
        if (error) *error = validationError;
        return false;
    }
    *result = parsed;
    return true;
}
