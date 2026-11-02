#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "infrastructure/variantarchive.h"

#include <QJsonObject>
#include <QMainWindow>
#include <optional>

class AutomatonEditor;
class QAction;
class QComboBox;
class QLabel;
class FeedbackLabel;
class TaskImage;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QShowEvent;
class QEvent;
class QCloseEvent;
class LogWindow;
class WorkScrollArea;
class HintSplitter;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    friend struct MainWindowTestAccess;
    QWidget* createLandingPage();
    QWidget* createWorkspacePage();
    QWidget* createStartPage();
    QWidget* createReportPage();
    void createMenus();
    void applyInitialSize();
    void fitStartPage();
    void refreshVariantInfo();
    void refreshArchiveInfo();
    void refreshTexts();
    void showResult(const Result& result);
    void displayResult(const Result& result);
    void openLog();
    void applyArchive(const QString& path, const QList<TrainingVariant>& variants);
    void chooseArchive();
    bool loadArchive(const QString& path, bool showErrors = true);
    void updateVariantSelection(int index);
    void beginWork();
    bool setupWorkspace(const TrainingVariant& variant);
    void continueWork();
    void returnToLanding();
    bool saveProgress(bool saveAs = false);
    void commitPendingInput();
    bool hasUnsavedChanges() const;
    bool confirmUnsavedChanges();
    QJsonObject workSnapshot() const;
    void openProgress();
    bool restoreProgress(const QJsonObject& data, const QString& progressPath);
    QJsonObject progressData() const;
    void createReport();
    QString reportHtml() const;
    void updateWorkflowState();

    QStackedWidget* m_pages;
    QLabel* m_archivePathLabel;
    QComboBox* m_variantCombo;
    QLabel* m_landingDirection;
    QPushButton* m_beginButton;

    QTabWidget* m_workflowTabs;
    QLabel* m_variantTitle;
    QLabel* m_startVariantLabel;
    QLabel* m_landingLogo;
    TaskImage* m_diagramLabel;
    QWidget* m_landingPage;
    WorkScrollArea* m_startWorkScroll;
    HintSplitter* m_startSplitter;
    AutomatonEditor* m_sourceEditor;
    AutomatonEditor* m_targetEditor;

    QLineEdit* m_teacherName;
    QLineEdit* m_studentName;
    QLineEdit* m_group;
    QLineEdit* m_recordBook;

    FeedbackLabel* m_notification;
    LogWindow* m_logWindow = nullptr;
    QAction* m_russianAction;
    QAction* m_englishAction;
    std::optional<Result> m_lastResult;
    bool m_ready = false;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QList<TrainingVariant> m_variants;
    TrainingVariant m_currentVariant;
    QString m_archivePath;
    QString m_progressPath;
    QJsonObject m_savedWorkState;
    bool m_initialSizeApplied = false;
    bool m_hasVariant = false;
    bool m_introCompleted = false;
};

#endif
