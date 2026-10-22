#ifndef AUTOMATON_H
#define AUTOMATON_H

#include <QJsonObject>
#include <QSet>
#include <QStringList>
#include <QVector>

enum class AutomatonKind {
    Mealy,
    Moore
};

struct Transition {
    int from = -1;
    int input = -1;
    int to = -1;
    QSet<int> outputs;
};

struct Automaton {
    AutomatonKind kind = AutomatonKind::Mealy;
    QStringList states;
    QStringList inputs;
    QStringList outputs;
    int initialState = -1;
    QVector<Transition> transitions;
    QVector<QSet<int>> mooreOutputs;

    const Transition* transition(int state, int input) const;
    bool validate(QString* error = nullptr) const;
    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject& object, Automaton* result,
                         QString* error = nullptr);
};

QString automatonKindName(AutomatonKind kind);

#endif
