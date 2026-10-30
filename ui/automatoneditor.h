#ifndef AUTOMATONEDITOR_H
#define AUTOMATONEDITOR_H

#include "domain/trainingvariant.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class AutomatonGraphEditor;
class QLabel;
class TaskImage;
class HintSplitter;
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
    void feedbackRequested(const QString& text, bool success);

private:
    struct CandidateCheck {
        bool accepted = false;
        QString explanation;
    };

    QWidget* createTablePage();
    QWidget* createMatrixPage();
    QWidget* createGraphPage();
    void configureTable();
    void fillEmptyCells(QTableWidget* table) const;
    int firstInputRow() const { return m_editorKind == AutomatonKind::Moore ? 4 : 2; }
    bool readTable(Automaton* result, QString* error) const;
    void configureMatrix();
    void updateReference(QTableWidget* table) const;
    bool readMatrix(Automaton* result, QString* error) const;
    bool readGraph(Automaton* result, QString* error) const;
    void acceptTable();
    void acceptMatrix();
    void acceptGraph();
    void invalidateFromTable();
    void invalidateFromMatrix();
    void invalidateGraph();
    void showFeedback(const QString& message, bool success);
    CandidateCheck checkCandidate(const Automaton& candidate, bool graph = false) const;
    QSet<int> parseOutputs(const QString& text, Automaton& automaton,
                           bool* ok) const;
    QJsonArray tableToJson(const QTableWidget* table) const;
    void tableFromJson(QTableWidget* table, const QJsonArray& array);

    TrainingVariant m_task;
    AutomatonKind m_editorKind = AutomatonKind::Mealy;
    Automaton m_tableAutomaton;
    Automaton m_matrixAutomaton;
    Automaton m_graphAutomaton;
    bool m_tableAccepted = false;
    bool m_matrixAccepted = false;
    bool m_graphAccepted = false;
    bool m_loading = false;

    QTabWidget* m_tabs;
    TaskImage* m_assignmentImage;
    HintSplitter* m_assignmentPanel;
    HintSplitter* m_matrixPanel;
    HintSplitter* m_graphPanel;
    QTableWidget* m_graphReference;
    QTableWidget* m_transitionTable;

    QLabel* m_matrixDimension;
    QLabel* m_outputCaption;
    QTableWidget* m_matrixOutputs;
    QTableWidget* m_matrixTable;
    QTableWidget* m_matrixReference;

    AutomatonGraphEditor* m_graphEditor;
};

#endif
