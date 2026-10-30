#include "domain/isomorphismchecker.h"

#include <QHash>

#include <algorithm>
#include <functional>

namespace {
struct Graph {
    QVector<QString> baseColors;
    QVector<QVector<QStringList>> edges;
};

void addVertex(Graph* graph, const QString& color) {
    const int oldSize = graph->baseColors.size();
    graph->baseColors.append(color);
    for (auto& row : graph->edges) row.resize(oldSize + 1);
    graph->edges.resize(oldSize + 1);
    graph->edges[oldSize].resize(oldSize + 1);
}

void addEdge(Graph* graph, int from, int to, const QString& role) {
    graph->edges[from][to].append(role);
    std::sort(graph->edges[from][to].begin(), graph->edges[from][to].end());
}

Graph makeGraph(const Automaton& automaton) {
    Graph graph;
    QVector<int> stateVertices, inputVertices, outputVertices;
    for (int i = 0; i < automaton.states.size(); ++i) {
        stateVertices.append(graph.baseColors.size());
        addVertex(&graph, i == automaton.initialState
                              ? QStringLiteral("state:initial")
                              : QStringLiteral("state"));
    }
    for (int i = 0; i < automaton.inputs.size(); ++i) {
        inputVertices.append(graph.baseColors.size());
        addVertex(&graph, QStringLiteral("input"));
    }
    for (int i = 0; i < automaton.outputs.size(); ++i) {
        outputVertices.append(graph.baseColors.size());
        addVertex(&graph, QStringLiteral("output"));
    }
    for (const Transition& transition : automaton.transitions) {
        const int vertex = graph.baseColors.size();
        addVertex(&graph, QStringLiteral("transition"));
        addEdge(&graph, stateVertices[transition.from], vertex, QStringLiteral("from"));
        addEdge(&graph, inputVertices[transition.input], vertex, QStringLiteral("input"));
        addEdge(&graph, vertex, stateVertices[transition.to], QStringLiteral("to"));
        for (int output : transition.outputs)
            addEdge(&graph, vertex, outputVertices[output], QStringLiteral("output"));
    }
    if (automaton.kind == AutomatonKind::Moore) {
        for (int state = 0; state < automaton.mooreOutputs.size(); ++state) {
            for (int output : automaton.mooreOutputs[state])
                addEdge(&graph, stateVertices[state], outputVertices[output],
                        QStringLiteral("state-output"));
        }
    }
    return graph;
}

QString signature(const Graph& graph, const QVector<QString>& colors, int vertex) {
    QStringList surroundings;
    for (int other = 0; other < graph.baseColors.size(); ++other) {
        for (const QString& edge : graph.edges[vertex][other])
            surroundings.append(QStringLiteral("O:%1:%2").arg(edge, colors[other]));
        for (const QString& edge : graph.edges[other][vertex])
            surroundings.append(QStringLiteral("I:%1:%2").arg(edge, colors[other]));
    }
    std::sort(surroundings.begin(), surroundings.end());
    return graph.baseColors[vertex] + QLatin1Char('|') +
           surroundings.join(QLatin1Char(';'));
}

bool refineTogether(const Graph& left, const Graph& right,
                    QVector<QString>* leftColors, QVector<QString>* rightColors) {
    *leftColors = left.baseColors;
    *rightColors = right.baseColors;
    for (int round = 0; round < left.baseColors.size() + 1; ++round) {
        QVector<QString> leftSignatures, rightSignatures;
        QStringList all;
        for (int i = 0; i < left.baseColors.size(); ++i) {
            leftSignatures.append(signature(left, *leftColors, i));
            all.append(leftSignatures.back());
        }
        for (int i = 0; i < right.baseColors.size(); ++i) {
            rightSignatures.append(signature(right, *rightColors, i));
            all.append(rightSignatures.back());
        }
        all.removeDuplicates();
        std::sort(all.begin(), all.end());
        QHash<QString, QString> ids;
        for (int i = 0; i < all.size(); ++i)
            ids.insert(all[i], QStringLiteral("c%1").arg(i));
        QVector<QString> newLeft, newRight;
        for (const QString& value : leftSignatures) newLeft.append(ids.value(value));
        for (const QString& value : rightSignatures) newRight.append(ids.value(value));
        if (newLeft == *leftColors && newRight == *rightColors) break;
        *leftColors = newLeft;
        *rightColors = newRight;
    }
    QHash<QString, int> balance;
    for (const QString& color : *leftColors) ++balance[color];
    for (const QString& color : *rightColors) --balance[color];
    for (int value : balance) if (value != 0) return false;
    return true;
}

bool edgeCompatible(const Graph& left, const Graph& right,
                    const QVector<int>& mapping, int leftVertex, int rightVertex) {
    if (left.edges[leftVertex][leftVertex] != right.edges[rightVertex][rightVertex])
        return false;
    for (int mappedLeft = 0; mappedLeft < mapping.size(); ++mappedLeft) {
        const int mappedRight = mapping[mappedLeft];
        if (mappedRight < 0) continue;
        if (left.edges[leftVertex][mappedLeft] != right.edges[rightVertex][mappedRight] ||
            left.edges[mappedLeft][leftVertex] != right.edges[mappedRight][rightVertex])
            return false;
    }
    return true;
}

bool isomorphic(const Graph& left, const Graph& right) {
    if (left.baseColors.size() != right.baseColors.size()) return false;
    QVector<QString> leftColors, rightColors;
    if (!refineTogether(left, right, &leftColors, &rightColors)) return false;
    QVector<int> mapping(left.baseColors.size(), -1);
    QVector<bool> used(right.baseColors.size(), false);

    std::function<bool(int)> search = [&](int mappedCount) {
        if (mappedCount == mapping.size()) return true;
        int chosen = -1;
        QVector<int> candidates;
        for (int leftVertex = 0; leftVertex < mapping.size(); ++leftVertex) {
            if (mapping[leftVertex] >= 0) continue;
            QVector<int> current;
            for (int rightVertex = 0; rightVertex < used.size(); ++rightVertex) {
                if (!used[rightVertex] && leftColors[leftVertex] == rightColors[rightVertex] &&
                    edgeCompatible(left, right, mapping, leftVertex, rightVertex))
                    current.append(rightVertex);
            }
            if (current.isEmpty()) return false;
            if (chosen < 0 || current.size() < candidates.size()) {
                chosen = leftVertex;
                candidates = current;
            }
        }
        for (int rightVertex : candidates) {
            mapping[chosen] = rightVertex;
            used[rightVertex] = true;
            if (search(mappedCount + 1)) return true;
            mapping[chosen] = -1;
            used[rightVertex] = false;
        }
        return false;
    };
    return search(0);
}
}

IsomorphismResult IsomorphismChecker::compare(const Automaton& expected,
                                              const Automaton& actual) {
    QString error;
    if (!actual.validate(&error))
        return {false, QCoreApplication::translate("Legacy", "Введённый автомат некорректен: %1").arg(error)};
    if (expected.kind != actual.kind)
        return {false, QCoreApplication::translate("Legacy", "Выбран неверный тип автомата.")};
    if (expected.states.size() != actual.states.size() ||
        expected.inputs.size() != actual.inputs.size() ||
        expected.outputs.size() != actual.outputs.size() ||
        expected.transitions.size() != actual.transitions.size()) {
        return {false, QCoreApplication::translate("Legacy", "Не совпадает число состояний, сигналов или переходов.")};
    }
    if (!isomorphic(makeGraph(expected), makeGraph(actual))) {
        return {false, QCoreApplication::translate("Legacy", "Нагруженные графы не изоморфны: проверьте начальное состояние, переходы и выходы.")};
    }
    return {true, QCoreApplication::translate("Legacy", "Верно. Автоматы изоморфны с точностью до переименования состояний и сигналов.")};
}
