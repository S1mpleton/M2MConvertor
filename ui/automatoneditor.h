#ifndef AUTOMATONEDITOR_H
#define AUTOMATONEDITOR_H

#include "domain/trainingvariant.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class AutomatonGraphEditor;
class QLabel;
class FeedbackLabel;
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
    int firstInputRow() const { return m_editorKind == AutomatonKind::Moore ? 4 : 2; }
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
    void showFeedback(FeedbackLabel* label, const QString& message, bool success);
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

    FeedbackLabel* m_notification;
    QTabWidget* m_tabs;
    QLabel* m_assignmentImage;
    QTableWidget* m_transitionTable;
    FeedbackLabel* m_tableFeedback;

    QLabel* m_matrixDimension;
    QLabel* m_outputCaption;
    QTableWidget* m_matrixOutputs;
    QTableWidget* m_matrixTable;
    QTableWidget* m_matrixReference;
    FeedbackLabel* m_matrixFeedback;

    AutomatonGraphEditor* m_graphEditor;
    FeedbackLabel* m_graphFeedback;
};

#endif
