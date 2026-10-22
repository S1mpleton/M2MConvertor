#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "infrastructure/variantarchive.h"

#include <QJsonObject>
#include <QMainWindow>

class AutomatonEditor;
class QAction;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTabWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* createLandingPage();
    QWidget* createWorkspacePage();
    QWidget* createStartPage();
    QWidget* createReportPage();
    void createMenus();
    void chooseArchive();
    bool loadArchive(const QString& path, bool showErrors = true);
    void updateVariantSelection(int index);
    void beginWork();
    void setupWorkspace(const TrainingVariant& variant);
    void acceptCounts();
    void returnToLanding();
    void saveProgress(bool saveAs = false);
    void openProgress();
    bool restoreProgress(const QJsonObject& data, const QString& progressPath);
    QJsonObject progressData() const;
    void createReport();
    void setFeedback(QLabel* label, const QString& text, bool success);
    void updateWorkflowState();

    QStackedWidget* m_pages;
    QLabel* m_archivePathLabel;
    QComboBox* m_variantCombo;
    QLabel* m_landingDirection;
    QPushButton* m_beginButton;

    QTabWidget* m_workflowTabs;
    QLabel* m_variantTitle;
    QLabel* m_directionLabel;
    QLabel* m_diagramLabel;
    QSpinBox* m_stateCount;
    QSpinBox* m_inputCount;
    QSpinBox* m_outputCount;
    QLabel* m_startFeedback;
    AutomatonEditor* m_sourceEditor;
    AutomatonEditor* m_targetEditor;

    QLineEdit* m_teacherName;
    QLineEdit* m_teacherId;
    QLineEdit* m_studentName;
    QLineEdit* m_studentId;
    QLineEdit* m_group;
    QLineEdit* m_recordBook;
    QLabel* m_reportFeedback;

    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QList<TrainingVariant> m_variants;
    TrainingVariant m_currentVariant;
    Automaton m_expectedTarget;
    QString m_archivePath;
    QString m_progressPath;
    bool m_hasVariant = false;
    bool m_countsAccepted = false;
};

#endif
