#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "infrastructure/variantarchive.h"

#include <QJsonObject>
#include <QMainWindow>

class AutomatonEditor;
class QAction;
class QComboBox;
class QLabel;
class FeedbackLabel;
class TaskImage;
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
    friend struct MainWindowTestAccess;
    QWidget* createLandingPage();
    QWidget* createWorkspacePage();
    QWidget* createStartPage();
    QWidget* createReportPage();
    void createMenus();
    void chooseArchive();
    bool loadArchive(const QString& path, bool showErrors = true);
    void updateVariantSelection(int index);
    void beginWork();
    bool setupWorkspace(const TrainingVariant& variant);
    void acceptCounts();
    void returnToLanding();
    void saveProgress(bool saveAs = false);
    void openProgress();
    bool restoreProgress(const QJsonObject& data, const QString& progressPath);
    QJsonObject progressData() const;
    void createReport();
    void setFeedback(const QString& text, bool success);
    QString reportHtml() const;
    void updateWorkflowState();

    QStackedWidget* m_pages;
    QLabel* m_archivePathLabel;
    QComboBox* m_variantCombo;
    QLabel* m_landingDirection;
    QPushButton* m_beginButton;

    QTabWidget* m_workflowTabs;
    QLabel* m_variantTitle;
    QLabel* m_directionLabel;
    TaskImage* m_diagramLabel;
    QSpinBox* m_stateCount;
    QSpinBox* m_inputCount;
    QSpinBox* m_outputCount;
    AutomatonEditor* m_sourceEditor;
    AutomatonEditor* m_targetEditor;

    QLineEdit* m_teacherName;
    QLineEdit* m_studentName;
    QLineEdit* m_group;
    QLineEdit* m_recordBook;

    FeedbackLabel* m_notification;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QList<TrainingVariant> m_variants;
    TrainingVariant m_currentVariant;
    QString m_archivePath;
    QString m_progressPath;
    bool m_hasVariant = false;
    bool m_countsAccepted = false;
};

#endif
