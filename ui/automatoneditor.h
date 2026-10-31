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
class QLineEdit;
class QSpinBox;

class AutomatonEditor : public QWidget {
    Q_OBJECT
public:
    explicit AutomatonEditor(QWidget* parent = nullptr);

    void setTask(const TrainingVariant& task, AutomatonKind kind);
    void resetEditor();
    void retranslateTexts();
    bool completed() const { return m_graphAccepted; }
    Automaton acceptedAutomaton() const { return m_graphAutomaton; }
    QJsonObject toJson() const;
    QJsonObject answerData() const;
    void fromJson(const QJsonObject& object);

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

signals:
    void completedChanged(bool completed);
    void feedbackRequested(const Result& result);

private:
    QString cellHelp(const QTableWidget* table, int row, int column) const;
    QWidget* createTablePage();
    QWidget* createMatrixPage();
    QWidget* createGraphPage();
    void configureTable();
    void configureMealyTable();
    void clearTable();
    bool mealyCountsMatch() const;
    void fillEmptyCells(QTableWidget* table) const;
    int firstInputRow() const { return m_editorKind == AutomatonKind::Moore ? 4 : 2; }
    bool readTable(Automaton* result, Message* error) const;
    void configureMatrix();
    void resetMatrix();
    void buildMatrix(int size);
    void clearMatrix();
    void updateReference(QTableWidget* table) const;
    bool readMatrix(Automaton* result, Message* error) const;
    bool readGraph(Automaton* result, Message* error) const;
    void acceptTable();
    void acceptMatrix();
    void acceptGraph();
    void invalidateFromTable();
    void invalidateFromMatrix();
    void invalidateGraph();
    void showFeedback(const Message& message, bool success, int page = 1);
    Message validationContext(int page) const;
    Result checkCandidate(const Automaton& candidate, bool graph = false) const;
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
    bool m_tableConfigured = false;

    QTabWidget* m_tabs;
    TaskImage* m_assignmentImage;
    HintSplitter* m_assignmentPanel;
    HintSplitter* m_matrixPanel;
    HintSplitter* m_graphPanel;
    QTableWidget* m_graphReference;
    QTableWidget* m_transitionTable;
    QWidget* m_tableContent;
    QWidget* m_mealySetup;
    QSpinBox* m_mealyStates;
    QSpinBox* m_mealyInputs;
    QSpinBox* m_mealyOutputs;
    QPushButton* m_clearTableButton;
    QPushButton* m_acceptTableButton;

    QSpinBox* m_matrixDimension;
    QWidget* m_matrixContent;
    QPushButton* m_clearMatrixButton;
    QPushButton* m_acceptMatrixButton;
    QLabel* m_outputCaption;
    QTableWidget* m_matrixOutputs;
    QTableWidget* m_matrixTable;
    QTableWidget* m_matrixReference;

    AutomatonGraphEditor* m_graphEditor;
};

#endif
