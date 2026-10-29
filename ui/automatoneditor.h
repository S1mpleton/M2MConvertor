#ifndef AUTOMATONEDITOR_H
#define AUTOMATONEDITOR_H

#include "domain/trainingvariant.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class AutomatonGraphEditor;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;

class AutomatonEditor : public QWidget {
    Q_OBJECT
public:
    explicit AutomatonEditor(QWidget* parent = nullptr);

    void setTask(const TrainingVariant& task, AutomatonKind kind);
    void resetEditor();
    bool completed() const { return m_graphAccepted; }
    Automaton acceptedAutomaton() const { return m_graphAutomaton; }
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& object);

signals:
    void completedChanged(bool completed);

private:
    struct CandidateCheck {
        bool accepted = false;
        QString explanation;
    };

    QWidget* createTablePage();
    QWidget* createMatrixPage();
    QWidget* createGraphPage();
    void configureTable();
    bool readTable(Automaton* result, QString* error) const;
    void configureMatrix();
    bool readMatrix(Automaton* result, QString* error) const;
    bool readGraph(Automaton* result, QString* error) const;
    void acceptTable();
    void acceptMatrix();
    void acceptGraph();
    void invalidateFromTable();
    void invalidateFromMatrix();
    void invalidateGraph();
    void showFeedback(QLabel* label, const QString& message, bool success);
    CandidateCheck checkCandidate(const Automaton& candidate) const;
    QStringList parsedNames(const QString& text) const;
    QSet<int> parseOutputs(const QString& text, Automaton& automaton,
                           bool* ok) const;
    QJsonArray tableToJson(const QTableWidget* table) const;
    void tableFromJson(QTableWidget* table, const QJsonArray& array);

    TrainingVariant m_task;
    QStringList m_tableInputs;
    AutomatonKind m_editorKind = AutomatonKind::Mealy;
    Automaton m_tableAutomaton;
    Automaton m_matrixAutomaton;
    Automaton m_graphAutomaton;
    bool m_tableAccepted = false;
    bool m_matrixAccepted = false;
    bool m_graphAccepted = false;
    bool m_loading = false;

    QTabWidget* m_tabs;
    QLineEdit* m_statesEdit;
    QLineEdit* m_inputsEdit;
    QLabel* m_assignmentImage;
    QLabel* m_typeLabel;
    QLabel* m_criterionLabel;
    QTableWidget* m_transitionTable;
    QLabel* m_tableFeedback;

    QLabel* m_matrixDimension;
    QTableWidget* m_matrixOutputs;
    QTableWidget* m_matrixTable;
    QTableWidget* m_matrixReference;
    QLabel* m_matrixFeedback;

    AutomatonGraphEditor* m_graphEditor;
    QLabel* m_graphFeedback;
};

#endif
