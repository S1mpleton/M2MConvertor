#ifndef AUTOMATONEDITOR_H
#define AUTOMATONEDITOR_H

#include "domain/automaton.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

class AutomatonCanvas;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTabWidget;

class AutomatonEditor : public QWidget {
    Q_OBJECT
public:
    explicit AutomatonEditor(QWidget* parent = nullptr);

    void setExpected(const Automaton& expected);
    void setEquivalentTo(const Automaton& source, AutomatonKind targetKind);
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
    void addGraphRow();
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
    QString outputsText(const QSet<int>& outputs, const Automaton& automaton) const;
    QSet<int> parseOutputs(const QString& text, const Automaton& automaton,
                           bool* ok) const;
    QJsonArray tableToJson(const QTableWidget* table) const;
    void tableFromJson(QTableWidget* table, const QJsonArray& array);

    Automaton m_reference;
    AutomatonKind m_editorKind = AutomatonKind::Mealy;
    bool m_checkBehavioralEquivalence = false;
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
    QLineEdit* m_outputsEdit;
    QComboBox* m_initialCombo;
    QLabel* m_typeLabel;
    QLabel* m_criterionLabel;
    QTableWidget* m_transitionTable;
    QLabel* m_tableFeedback;

    QSpinBox* m_matrixDimension;
    QTableWidget* m_matrixTable;
    QLabel* m_matrixFeedback;

    QTableWidget* m_edgeTable;
    AutomatonCanvas* m_canvas;
    QLabel* m_graphFeedback;
};

#endif
