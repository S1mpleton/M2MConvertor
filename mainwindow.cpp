#include "infrastructure/logger.h"
#include "infrastructure/translationmanager.h"
#include "ui/logwindow.h"
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include "ui/uitext.h"
#include "mainwindow.h"

#include "infrastructure/progressstore.h"
#include "ui/automatoneditor.h"
#include "ui/widgets/taskimage.h"
#include "ui/widgets/hintsplitter.h"
#include "ui/widgets/wrappinglabel.h"
#include <QShowEvent>
#include <QTimer>
#include "ui/widgets/workscrollarea.h"
#include "ui/widgets/actionbar.h"
#include "ui/workflowtabs.h"
#include "ui/appicons.h"

#include <QAction>
#include <QBuffer>
#include <QButtonGroup>
#include <QDir>
#include <QSpinBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QKeySequence>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
#include <QStackedWidget>
#include <QStyle>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>

#include <algorithm>

namespace {
constexpr int kLandingPage = 0;
constexpr int kWorkspacePage = 1;

QString automatonSummary(const Automaton& automaton) {
    return QCoreApplication::translate("Report", "%1: %2 состояний, %3 входов, %4 выходов, %5 переходов")
        .arg(automatonKindName(automaton.kind))
        .arg(automaton.states.size()).arg(automaton.inputs.size())
        .arg(automaton.outputs.size()).arg(automaton.transitions.size());
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowIcon(AppIcons::applicationIcon());
    UiText::bind(this, "windowTitle", QT_TRANSLATE_NOOP("App", "M2MConvertor — эквивалентные автоматы"));
    m_pages = new QStackedWidget;
    m_pages->addWidget(createLandingPage());
    m_pages->addWidget(createWorkspacePage());
    auto* central = new QWidget;
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0); centralLayout->setSpacing(0);
    centralLayout->addWidget(m_pages, 1);
    setCentralWidget(central);
    createMenus();
    for (auto* editor : {m_sourceEditor, m_targetEditor})
        connect(editor, &AutomatonEditor::feedbackRequested, this, &MainWindow::showResult);
    connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this, &MainWindow::refreshTexts);
    m_ready = true;
    refreshTexts();
    Logger::instance().log(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Приложение запущено."))));
    applyInitialSize();
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (m_initialSizeApplied) return;
    m_initialSizeApplied = true;
    QTimer::singleShot(0, this, &MainWindow::applyInitialSize);
}

void MainWindow::applyInitialSize() {
    layout()->activate();
    const auto available = screen()->availableGeometry();
    const auto decoration = frameGeometry().size() - size();
    const auto limit = available.size() - decoration;
    const int preferredWidth = qMax(minimumSizeHint().width(), qMax(640, fontMetrics().averageCharWidth() * 110));
    const int width = qMin(preferredWidth, limit.width());
    resize(width, qMin(qMax(minimumSizeHint().height(), 480), limit.height()));
    layout()->activate();
    const auto* landingLayout = m_landingPage->layout();
    const int landingHeight = qMax(landingLayout->minimumSize().height(), landingLayout->minimumHeightForWidth(width));
    resize(width, qMin(qMax(minimumSizeHint().height(), landingHeight + menuBar()->height()), limit.height()));
    if (isVisible()) {
        const auto offset = pos() - frameGeometry().topLeft();
        move(available.center() - QPoint(frameGeometry().width() / 2, frameGeometry().height() / 2) + offset);
    }
}

QWidget* MainWindow::createLandingPage() {
    m_landingPage = new QWidget;
    auto* outer = new QVBoxLayout(m_landingPage);
    outer->setContentsMargins(32, 28, 32, 28);
    outer->addStretch();
    auto* content = new QWidget;
    content->setObjectName("landingContent");
    content->setMaximumWidth(680);
    auto* choices = new QVBoxLayout(content);
    choices->setContentsMargins(0, 0, 0, 0);
    choices->setSpacing(18);
    m_directionButtons = new QButtonGroup(this);
    m_directionButtons->setExclusive(true);
    for (const auto direction : {ConversionDirection::MooreToMealy, ConversionDirection::MealyToMoore}) {
        auto* button = UiText::make<QPushButton>(direction == ConversionDirection::MooreToMealy
            ? QT_TRANSLATE_NOOP("App", "Мура → Мили") : QT_TRANSLATE_NOOP("App", "Мили → Мура"));
        button->setObjectName(direction == ConversionDirection::MooreToMealy ? "mooreToMealyButton" : "mealyToMooreButton");
        button->setCheckable(true);
        button->setMinimumHeight(42);
        m_directionButtons->addButton(button, static_cast<int>(direction));
        choices->addWidget(button);
    }
    choices->addSpacing(32);
    auto* actions = new QHBoxLayout;
    actions->setSpacing(18);
    m_beginButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Создать"));
    m_beginButton->setObjectName("beginWorkButton");
    m_beginButton->setDefault(true);
    auto* open = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Открыть"));
    open->setObjectName("openProgressButton");
    for (auto* button : {m_beginButton, open}) button->setMinimumHeight(42);
    actions->addWidget(m_beginButton, 1);
    actions->addWidget(open, 1);
    choices->addLayout(actions);
    auto* centered = new QHBoxLayout;
    centered->addStretch(1);
    centered->addWidget(content, 4);
    centered->addStretch(1);
    outer->addLayout(centered);
    outer->addStretch();
    connect(m_beginButton, &QPushButton::clicked, this, &MainWindow::beginWork);
    connect(open, &QPushButton::clicked, this, &MainWindow::openProgress);
    connect(m_directionButtons, &QButtonGroup::idClicked, this, [this] { refreshTexts(); });
    return m_landingPage;
}

QWidget* MainWindow::createWorkspacePage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 10, 10, 10);
    auto* top = new QHBoxLayout;
    auto* back = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "К выбору преобразования"));
    back->setIcon(AppIcons::icon(AppIcons::Kind::Back));
    m_variantTitle = new WrappingLabel;
    QFont font = m_variantTitle->font();
    font.setPointSize(13);
    font.setBold(true);
    m_variantTitle->setFont(font);
    top->addWidget(back);
    top->addWidget(m_variantTitle);
    top->addStretch();
    layout->addLayout(top);
    m_workflowTabs = new WorkflowTabs;
    m_workflowTabs->setObjectName(QStringLiteral("workflowTabs"));
    UiText::tab(m_workflowTabs, createStartPage(), QT_TRANSLATE_NOOP("App", "Выбор варианта"));
    m_sourceEditor = new AutomatonEditor;
    m_sourceEditor->setObjectName("sourceEditor");
    m_targetEditor = new AutomatonEditor;
    m_targetEditor->setObjectName("targetEditor");
    m_workflowTabs->addTab(m_sourceEditor, QString());
    m_workflowTabs->addTab(m_targetEditor, QString());
    UiText::tab(m_workflowTabs, createReportPage(), QT_TRANSLATE_NOOP("App", "Отчёт"));
    m_workflowTabs->setTabEnabled(1, false);
    m_workflowTabs->setTabEnabled(2, false);
    m_workflowTabs->setTabEnabled(3, false);
    layout->addWidget(m_workflowTabs, 1);
    auto* navigation = new ActionBar(Qt::AlignHCenter);
    navigation->setObjectName("workNavigation");
    m_previousStepButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "← Назад"));
    m_previousStepButton->setObjectName("previousStepButton");
    m_saveStepButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Сохранить"));
    m_saveStepButton->setObjectName("saveStepButton");
    m_nextStepButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Далее →"));
    m_nextStepButton->setObjectName("nextStepButton");
    for (auto* button : {m_previousStepButton, m_saveStepButton, m_nextStepButton}) {
        button->setMinimumWidth(fontMetrics().horizontalAdvance("MMMMMMMMMM"));
        navigation->addWidget(button);
    }
    layout->addWidget(navigation);
    connect(m_previousStepButton, &QPushButton::clicked, this, &MainWindow::previousStep);
    connect(m_saveStepButton, &QPushButton::clicked, this, [this] { saveProgress(); });
    connect(m_nextStepButton, &QPushButton::clicked, this, &MainWindow::nextStep);
    connect(m_workflowTabs, &QTabWidget::currentChanged, this, &MainWindow::updateNavigation);
    connect(back, &QPushButton::clicked, this, &MainWindow::returnToLanding);
    connect(m_sourceEditor, &AutomatonEditor::completedChanged, this, [this](bool complete) {
        if (!complete) m_targetEditor->resetEditor();
        updateWorkflowState();
    });
    connect(m_targetEditor, &AutomatonEditor::completedChanged,
            this, [this] { updateWorkflowState(); });
    return page;
}

QWidget* MainWindow::createStartPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);
    auto* archiveRow = new QHBoxLayout;
    m_archivePathLabel = new WrappingLabel;
    m_archivePathLabel->setObjectName("archiveSummary");
    m_archivePathLabel->setTextFormat(Qt::PlainText);
    auto* archiveButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Открыть базу вариантов…"));
    m_openArchiveButton = archiveButton;
    archiveButton->hide();
    archiveButton->setObjectName("openArchiveButton");
    archiveButton->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    archiveRow->addWidget(m_archivePathLabel, 1);
    archiveRow->addWidget(archiveButton);
    layout->addLayout(archiveRow);
    m_variantCountLabel = new QLabel;
    m_variantCountLabel->setObjectName("variantCountLabel");
    layout->addWidget(m_variantCountLabel);
    auto* selection = new QHBoxLayout;
    selection->addWidget(UiText::make<QLabel>(QT_TRANSLATE_NOOP("App", "Введите номер варианта:")));
    m_variantNumber = new QSpinBox;
    m_variantNumber->setObjectName("variantNumber");
    m_variantNumber->setRange(1, 999999);
    selection->addWidget(m_variantNumber, 1);
    m_acceptVariantButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять"));
    m_acceptVariantButton->setObjectName("continueWorkButton");
    selection->addWidget(m_acceptVariantButton);
    layout->addLayout(selection);
    m_landingDirection = new WrappingLabel;
    m_landingDirection->setObjectName("selectionDirection");
    layout->addWidget(m_landingDirection);
    m_diagramLabel = new TaskImage;
    m_diagramLabel->setObjectName("startTaskImage");
    auto* explanation = new QWidget;
    explanation->setObjectName("startWorkArea");
    auto* textLayout = new QVBoxLayout(explanation);
    textLayout->setContentsMargins(0, 0, 0, 0);
    m_workInstructions = new WrappingLabel;
    m_workInstructions->setObjectName("workInstructions");
    textLayout->addWidget(m_workInstructions);
    textLayout->addStretch();
    m_startSplitter = new HintSplitter(UiText::presentationTitle(true), m_diagramLabel, explanation,
                                      "startAssignment", Qt::Vertical);
    m_startSplitter->widget(0)->layout()->setSizeConstraint(QLayout::SetMinimumSize);
    m_startSplitter->setWorkTitle(QT_TRANSLATE_NOOP("App", "Пояснения к работе"));
    m_startSplitter->hide();
    layout->addWidget(m_startSplitter, 1);
    layout->addStretch();
    connect(archiveButton, &QPushButton::clicked, this, &MainWindow::chooseArchive);
    connect(m_acceptVariantButton, &QPushButton::clicked, this, &MainWindow::continueWork);
    connect(m_variantNumber, &QSpinBox::valueChanged, this, &MainWindow::updateVariantSelection);
    return page;
}

QWidget* MainWindow::createReportPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* teacher = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "Преподаватель"));
    auto* teacherForm = new QFormLayout(teacher);
    m_teacherName = new QLineEdit;
    UiText::row(teacherForm, QT_TRANSLATE_NOOP("App", "Ф. И. О. преподавателя:"), m_teacherName);
    auto* student = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "Студент"));
    auto* studentForm = new QFormLayout(student);
    m_studentName = new QLineEdit;
    m_group = new QLineEdit;
    m_recordBook = new QLineEdit;
    UiText::row(studentForm, QT_TRANSLATE_NOOP("App", "Ф. И. О. студента:"), m_studentName);
    UiText::row(studentForm, QT_TRANSLATE_NOOP("App", "Группа:"), m_group);
    UiText::row(studentForm, QT_TRANSLATE_NOOP("App", "Номер зачётной книжки:"), m_recordBook);
    auto* create = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Создать отчёт HTML…"));
    create->setObjectName(QStringLiteral("createReportButton"));
    create->setIcon(AppIcons::icon(AppIcons::Kind::Save));
    layout->addWidget(teacher);
    layout->addWidget(student);
    layout->addStretch();
    connect(create, &QPushButton::clicked, this, &MainWindow::createReport);
    auto* container = new QWidget;
    auto* containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(10, 10, 10, 10);
    containerLayout->addWidget(new WorkScrollArea(page), 1);
    auto* actions = new ActionBar;
    actions->addWidget(create);
    containerLayout->addWidget(actions);
    return container;
}

void MainWindow::createMenus() {
    AppIcons::configureMenuBar(menuBar());
    auto* file = UiText::menu(menuBar(), AppIcons::icon(AppIcons::Kind::File), QT_TRANSLATE_NOOP("App", "Файл"));
    auto* newWork = UiText::action(file, AppIcons::icon(AppIcons::Kind::New), QT_TRANSLATE_NOOP("App", "Выбрать другой вариант"));
    auto* open = UiText::action(file, AppIcons::icon(AppIcons::Kind::Open), QT_TRANSLATE_NOOP("App", "Открыть работу…"));
    m_saveAction = UiText::action(file, AppIcons::icon(AppIcons::Kind::Save), QT_TRANSLATE_NOOP("App", "Сохранить работу"));
    m_saveAction->setObjectName("saveWorkAction");
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAsAction = UiText::action(file, AppIcons::icon(AppIcons::Kind::Save), QT_TRANSLATE_NOOP("App", "Сохранить работу как…"));
    file->addSeparator();
    auto* quit = UiText::action(file, AppIcons::icon(AppIcons::Kind::Exit), QT_TRANSLATE_NOOP("App", "Выход"));
    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);
    auto* settings = UiText::menu(menuBar(), AppIcons::icon(AppIcons::Kind::Settings), QT_TRANSLATE_NOOP("App", "Настройки"));
    settings->setObjectName("settingsMenu");
    auto* logAction = UiText::action(settings, AppIcons::icon(AppIcons::Kind::Log), QT_TRANSLATE_NOOP("App", "Журнал событий"));
    logAction->setObjectName("eventLogAction");
    connect(logAction, &QAction::triggered, this, &MainWindow::openLog);
    auto* languages = settings->addMenu(AppIcons::icon(AppIcons::Kind::Language), QString());
    UiText::bind(languages, "title", QT_TRANSLATE_NOOP("App", "Язык"));
    auto* group = new QActionGroup(this);
    m_russianAction = languages->addAction(AppIcons::icon(AppIcons::Kind::Russian), QStringLiteral("Русский"));
    m_englishAction = languages->addAction(AppIcons::icon(AppIcons::Kind::English), QStringLiteral("English"));
    m_russianAction->setObjectName("languageRussianAction"); m_englishAction->setObjectName("languageEnglishAction");
    for (auto* action : {m_russianAction, m_englishAction}) { action->setCheckable(true); group->addAction(action); }
    connect(m_russianAction, &QAction::triggered, this, [this] { showResult(TranslationManager::instance().setLocale("ru_RU")); });
    connect(m_englishAction, &QAction::triggered, this, [this] { showResult(TranslationManager::instance().setLocale("en")); });
    connect(newWork, &QAction::triggered, this, &MainWindow::returnToLanding);
    connect(open, &QAction::triggered, this, &MainWindow::openProgress);
    connect(m_saveAction, &QAction::triggered, this, [this] { saveProgress(false); });
    connect(m_saveAsAction, &QAction::triggered, this, [this] { saveProgress(true); });
    connect(quit, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::chooseArchive() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Открыть базу вариантов"), QString(), tr("База M2MTools (*.db *.m2m);;Все файлы (*)"));
    if (!path.isEmpty()) loadArchive(path);
}

bool MainWindow::loadArchive(const QString& path, bool showErrors) {
    QList<TrainingVariant> variants;
    const auto result = VariantArchive::read(path, &variants);
    if (!result.ok()) {
        if (showErrors) showResult(result); else Logger::instance().log(result);
        return false;
    }
    if (variants.isEmpty()) {
        const auto empty = Result::warning(Message(QT_TRANSLATE_NOOP("Result", "В BazaVar нет вариантов.")), ResultCategory::Archive);
        if (showErrors) showResult(empty); else Logger::instance().log(empty);
        return false;
    }
    const auto direction = selectedDirection();
    if (direction && std::none_of(variants.cbegin(), variants.cend(), [&](const auto& variant) { return variant.direction == *direction; })) {
        const auto missing = Result::warning(Message(QT_TRANSLATE_NOOP("Result", "В базе нет вариантов выбранного преобразования.")), ResultCategory::Archive);
        if (showErrors) showResult(missing); else Logger::instance().log(missing);
        return false;
    }
    if (!confirmUnsavedChanges()) return false;
    applyArchive(path, variants);
    if (showErrors) showResult(result); else Logger::instance().log(result);
    return true;
}

bool MainWindow::tryOpenArchive(const QString& directory) {
    const QDir folder(directory);
    QStringList files;
    for (const auto& name : {QStringLiteral("database.db"), QStringLiteral("BazaVar.m2m")})
        if (folder.exists(name)) files.append(name);
    for (const auto& entry : folder.entryInfoList(QDir::Files | QDir::Readable, QDir::Name)) {
        const QString suffix = entry.suffix().toLower();
        if ((suffix == "db" || suffix == "m2m") && !files.contains(entry.fileName()))
            files.append(entry.fileName());
    }
    const auto direction = selectedDirection();
    for (const auto& name : files) {
        QList<TrainingVariant> variants;
        const QString path = folder.filePath(name);
        const auto result = VariantArchive::read(path, &variants);
        Logger::instance().log(result);
        if (!result.ok() || variants.isEmpty()) continue;
        if (direction && std::none_of(variants.cbegin(), variants.cend(), [&](const auto& variant) {
            return variant.direction == *direction;
        })) continue;
        applyArchive(path, variants);
        m_autoArchiveFailed = false;
        m_openArchiveButton->hide();
        return true;
    }
    m_autoArchiveFailed = true;
    m_openArchiveButton->show();
    return false;
}

void MainWindow::applyArchive(const QString& path, const QList<TrainingVariant>& variants) {
    m_archivePath = QFileInfo(path).absoluteFilePath();
    m_autoArchiveFailed = false;
    m_openArchiveButton->hide();
    m_variants = variants;
    m_hasVariant = false;
    m_introCompleted = false;
    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);
    const auto direction = selectedDirection();
    const auto first = std::find_if(variants.cbegin(), variants.cend(), [&](const auto& variant) {
        return direction && variant.direction == *direction;
    });
    if (first != variants.cend()) m_variantNumber->setValue(first->number);
    refreshArchiveInfo();
    updateVariantSelection(m_variantNumber->value());
}

std::optional<ConversionDirection> MainWindow::selectedDirection() const {
    const int id = m_directionButtons->checkedId();
    if (id < 0) return std::nullopt;
    return static_cast<ConversionDirection>(id);
}

void MainWindow::selectDirection(ConversionDirection direction) {
    if (auto* button = m_directionButtons->button(static_cast<int>(direction))) button->setChecked(true);
    refreshTexts();
}

void MainWindow::updateVariantSelection(int) {
    const auto direction = selectedDirection();
    const int count = std::count_if(m_variants.cbegin(), m_variants.cend(), [&](const auto& variant) {
        return direction && variant.direction == *direction;
    });
    m_variantCountLabel->setText(tr("Количество вариантов в базе данных: %1").arg(m_variants.size()));
    m_landingDirection->setText(direction ? conversionDirectionName(*direction) : QString());
    m_variantNumber->setEnabled(count > 0);
    m_acceptVariantButton->setEnabled(count > 0);
    const bool accepted = m_hasVariant && m_introCompleted && direction == m_currentVariant.direction &&
        m_variantNumber->value() == m_currentVariant.number;
    m_startSplitter->setVisible(accepted);
    updateWorkflowState();
}

void MainWindow::beginWork() {
    const auto direction = selectedDirection();
    if (!direction) {
        showResult(Result::warning(Message(QT_TRANSLATE_NOOP("Result", "Выберите тип преобразования")), ResultCategory::Validation));
        return;
    }
    if (!confirmUnsavedChanges()) return;
    m_hasVariant = false;
    m_introCompleted = false;
    m_progressPath.clear();
    m_saveAction->setEnabled(false); m_saveAsAction->setEnabled(false);
    if (std::none_of(m_variants.cbegin(), m_variants.cend(), [&](const auto& variant) { return variant.direction == *direction; })) {
        m_variants.clear(); m_archivePath.clear();
        tryOpenArchive(QCoreApplication::applicationDirPath());
    }
    const auto first = std::find_if(m_variants.cbegin(), m_variants.cend(), [&](const auto& variant) {
        return variant.direction == *direction;
    });
    if (first != m_variants.cend()) m_variantNumber->setValue(first->number);
    m_variantTitle->setText(conversionDirectionName(*direction));
    m_workflowTabs->setCurrentIndex(0);
    m_pages->setCurrentIndex(kWorkspacePage);
    updateVariantSelection(m_variantNumber->value());
    refreshVariantInfo();
}

bool MainWindow::setupWorkspace(const TrainingVariant& variant) {
    if (!variant.available()) return false;
    m_currentVariant = variant;
    m_hasVariant = true;
    m_introCompleted = false;
    selectDirection(variant.direction);
    m_variantNumber->setValue(variant.number);
    m_variantTitle->setText(tr("Вариант №%1 — %2")
        .arg(variant.number).arg(conversionDirectionName(variant.direction)));
    refreshVariantInfo();
    m_diagramLabel->setImageData(variant.diagram);
    m_sourceEditor->setTask(variant, variant.sourceKind());
    m_targetEditor->setTask(variant, variant.targetKind());
    m_workflowTabs->setCurrentIndex(0);
    m_teacherName->clear(); m_studentName->clear();
    m_group->clear(); m_recordBook->clear();
    m_saveAction->setEnabled(true); m_saveAsAction->setEnabled(true);
    refreshTexts();
    updateWorkflowState();
    m_savedWorkState = workSnapshot();
    return true;
}

void MainWindow::fitStartPage() {
    if (m_workflowTabs->currentIndex() != 0 || !m_introCompleted) return;
    m_startSplitter->setHintExtent(qMax(120, m_startSplitter->height() * 3 / 4));
}

void MainWindow::continueWork() {
    const auto direction = selectedDirection();
    const int number = m_variantNumber->value();
    const auto found = std::find_if(m_variants.cbegin(), m_variants.cend(), [&](const auto& variant) {
        return direction && variant.direction == *direction && variant.number == number;
    });
    if (found == m_variants.cend()) {
        showResult(Result::warning(Message(QT_TRANSLATE_NOOP("Result", "Вариант №%1 для выбранного преобразования не найден.")).arg(number), ResultCategory::Validation));
        return;
    }
    if (!(m_hasVariant && m_currentVariant.number == number && m_currentVariant.direction == *direction)) {
        if (!confirmUnsavedChanges() || !setupWorkspace(*found)) return;
        m_progressPath.clear();
        m_introCompleted = true;
        m_savedWorkState = workSnapshot();
        showResult(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Начата работа с вариантом №%1.")).arg(number), ResultCategory::Progress));
    }
    m_introCompleted = true;
    updateVariantSelection(number);
    m_workflowTabs->setCurrentIndex(0);
    QTimer::singleShot(0, this, &MainWindow::fitStartPage);
}

void MainWindow::refreshArchiveInfo() {
    m_archivePathLabel->setText(m_archivePath.isEmpty() ? tr("База вариантов не открыта") :
        tr("%1 — вариантов: %2").arg(QFileInfo(m_archivePath).fileName()).arg(m_variants.size()));
    m_archivePathLabel->setToolTip(m_archivePath);
    m_openArchiveButton->setVisible(m_autoArchiveFailed && m_variants.isEmpty());
}

void MainWindow::refreshVariantInfo() {
    const auto direction = selectedDirection().value_or(ConversionDirection::MooreToMealy);
    const bool moore = direction == ConversionDirection::MooreToMealy;
    m_startSplitter->setHintTitle(UiText::presentationTitle(moore));
    UiText::bind(m_workInstructions, "text", moore
        ? QT_TRANSLATE_NOOP("App", "Преобразовать частично заданный автомат Мура, представленный в теоретико-множественной форме, в эквивалентный ему автомат Мили.")
        : QT_TRANSLATE_NOOP("App", "Преобразовать частично заданный автомат Мили, представленный в теоретико-множественной форме, в эквивалентный ему автомат Мура."));
    m_workflowTabs->setTabText(1, UiText::text(moore ? QT_TRANSLATE_NOOP("App", "Автомат Мура") : QT_TRANSLATE_NOOP("App", "Автомат Мили")));
    m_workflowTabs->setTabText(2, UiText::text(moore ? QT_TRANSLATE_NOOP("App", "Автомат Мили") : QT_TRANSLATE_NOOP("App", "Автомат Мура")));
}

void MainWindow::updateWorkflowState() {
    const bool accepted = m_hasVariant && m_introCompleted && selectedDirection() == m_currentVariant.direction &&
        m_variantNumber->value() == m_currentVariant.number;
    m_workflowTabs->setTabEnabled(1, accepted);
    const bool sourceComplete = accepted && m_sourceEditor->completed();
    m_workflowTabs->setTabEnabled(2, sourceComplete);
    m_workflowTabs->setTabEnabled(3, sourceComplete && m_targetEditor->completed());
    updateNavigation();
}

void MainWindow::updateNavigation() {
    m_saveStepButton->setEnabled(m_hasVariant);
    m_nextStepButton->setEnabled(m_workflowTabs->currentIndex() != 3);
}

void MainWindow::previousStep() {
    commitPendingInput();
    const int page = m_workflowTabs->currentIndex();
    if (page == 0) {
        returnToLanding();
    } else if (page == 3) {
        m_targetEditor->selectStage(2);
        m_workflowTabs->setCurrentIndex(2);
    } else {
        auto* editor = page == 1 ? m_sourceEditor : m_targetEditor;
        if (editor->currentStage() > 0) editor->selectStage(editor->currentStage() - 1);
        else {
            if (page == 2) m_sourceEditor->selectStage(2);
            m_workflowTabs->setCurrentIndex(page - 1);
        }
    }
}

void MainWindow::nextStep() {
    commitPendingInput();
    const int page = m_workflowTabs->currentIndex();
    if (page == 0) {
        if (!m_workflowTabs->isTabEnabled(1)) continueWork();
        if (m_workflowTabs->isTabEnabled(1)) {
            m_sourceEditor->selectStage(0);
            m_workflowTabs->setCurrentIndex(1);
        }
    } else if (page == 1 || page == 2) {
        auto* editor = page == 1 ? m_sourceEditor : m_targetEditor;
        if (editor->advance()) {
            updateWorkflowState();
            if (m_workflowTabs->isTabEnabled(page + 1)) {
                if (page == 1) m_targetEditor->selectStage(0);
                m_workflowTabs->setCurrentIndex(page + 1);
            }
        }
    }
}

void MainWindow::returnToLanding() {
    if (!confirmUnsavedChanges()) return;
    Logger::instance().log(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Возврат к выбору варианта.")), ResultCategory::Progress));
    m_pages->setCurrentIndex(kLandingPage);
    m_hasVariant = false;
    m_introCompleted = false;
    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);
}

QJsonObject MainWindow::progressData() const {
    QJsonObject data;
    data.insert(QStringLiteral("archivePath"), m_archivePath);
    data.insert(QStringLiteral("variantNumber"), m_currentVariant.number);
    data.insert("direction", m_currentVariant.direction == ConversionDirection::MooreToMealy ? "MooreToMealy" : "MealyToMoore");
    data.insert("introCompleted", m_introCompleted);
    data.insert(QStringLiteral("source"), m_sourceEditor->toJson());
    data.insert(QStringLiteral("target"), m_targetEditor->toJson());
    QJsonObject report;
    report.insert(QStringLiteral("teacherName"), m_teacherName->text());
    report.insert(QStringLiteral("studentName"), m_studentName->text());
    report.insert(QStringLiteral("group"), m_group->text());
    report.insert(QStringLiteral("recordBook"), m_recordBook->text());
    data.insert(QStringLiteral("report"), report);
    return data;
}

QJsonObject MainWindow::workSnapshot() const {
    auto data = progressData();
    data.insert("source", m_sourceEditor->answerData());
    data.insert("target", m_targetEditor->answerData());
    return data;
}

void MainWindow::commitPendingInput() {
    m_sourceEditor->commitPendingInput();
    m_targetEditor->commitPendingInput();
    if (auto* focus = QApplication::focusWidget())
        if (m_sourceEditor->isAncestorOf(focus) || m_targetEditor->isAncestorOf(focus))
            m_workflowTabs->setFocus();
}

bool MainWindow::hasUnsavedChanges() const {
    return m_hasVariant && workSnapshot() != m_savedWorkState;
}

bool MainWindow::confirmUnsavedChanges() {
    commitPendingInput();
    if (!hasUnsavedChanges()) return true;
    QMessageBox box(QMessageBox::Warning, tr("Несохранённые изменения"),
        tr("В работе есть несохранённые изменения. Сохранить их перед продолжением?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
    box.setObjectName("unsavedChangesDialog");
    box.setInformativeText(tr("Если продолжить без сохранения, изменения будут потеряны."));
    box.button(QMessageBox::Save)->setText(tr("Сохранить"));
    box.button(QMessageBox::Discard)->setText(tr("Не сохранять"));
    box.button(QMessageBox::Cancel)->setText(tr("Отмена"));
    box.setDefaultButton(QMessageBox::Save);
    box.setEscapeButton(QMessageBox::Cancel);
    switch (box.exec()) {
    case QMessageBox::Save: return saveProgress();
    case QMessageBox::Discard: return true;
    default: return false;
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmUnsavedChanges()) QMainWindow::closeEvent(event);
    else event->ignore();
}

bool MainWindow::saveProgress(bool saveAs) {
    if (!m_hasVariant) return false;
    commitPendingInput();
    QString path = m_progressPath;
    const bool legacyCollision = !path.isEmpty() && ProgressStore::savePath(path) != path &&
                                 QFileInfo::exists(ProgressStore::savePath(path));
    if (saveAs || path.isEmpty() || legacyCollision) {
        const QString folder = ProgressStore::workDirectory();
        QDir().mkpath(folder);
        const QString extension = ProgressStore::suffix();
        path = QFileDialog::getSaveFileName(
            this, tr("Сохранить учебную работу"),
            legacyCollision ? ProgressStore::savePath(path) :
                QDir(folder).filePath(tr("variant-%1.%2").arg(m_currentVariant.number).arg(extension)),
            tr("Работа M2MConvertor (*.%1)").arg(extension));
        if (path.isEmpty()) return false;
    }
    path = ProgressStore::savePath(path);
    const auto result = ProgressStore::write(path, progressData());
    showResult(result);
    if (result.ok()) {
        m_progressPath = path;
        m_savedWorkState = workSnapshot();
    }
    return result.ok();
}

void MainWindow::openProgress() {
    const QString folder = ProgressStore::workDirectory();
    QDir().mkpath(folder);
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Открыть учебную работу"), folder,
        tr("Работа M2MConvertor (*.m2ms)"));
    if (path.isEmpty()) return;
    QJsonObject data;
    const auto result = ProgressStore::read(path, &data);
    if (!result.ok()) { showResult(result); return; }
    if (!restoreProgress(data, path)) return;
    m_pages->setCurrentIndex(kWorkspacePage);
    showResult(result);
}

bool MainWindow::restoreProgress(const QJsonObject& data, const QString& progressPath) {
    QString archivePath = data.value(QStringLiteral("archivePath")).toString();
    QList<TrainingVariant> variants;
    auto archiveResult = VariantArchive::read(archivePath, &variants);
    if (!archiveResult.ok()) {
        Logger::instance().log(archiveResult);
        archivePath = QFileDialog::getOpenFileName(
            this, tr("Укажите BazaVar для сохранённой работы"), QString(),
            tr("База M2MTools (*.db *.m2m);;Все файлы (*)"));
        if (archivePath.isEmpty()) return false;
        archiveResult = VariantArchive::read(archivePath, &variants);
        if (!archiveResult.ok()) { showResult(archiveResult); return false; }
    }
    const int variantNumber = data.value(QStringLiteral("variantNumber")).toInt();
    auto found = std::find_if(variants.cbegin(), variants.cend(),
        [variantNumber](const TrainingVariant& variant) { return variant.number == variantNumber; });
    if (found == variants.cend()) {
        showResult(Result::error(Message(QT_TRANSLATE_NOOP("Result", "В текущем BazaVar нет варианта №%1.")).arg(variantNumber), ResultCategory::Progress));
        return false;
    }
    const auto fileDirection = ProgressStore::directionFromPath(progressPath);
    const QString declared = data.value("direction").toString();
    const QString expected = found->direction == ConversionDirection::MooreToMealy ? "MooreToMealy" : "MealyToMoore";
    if ((fileDirection && *fileDirection != found->direction) || (!declared.isEmpty() && declared != expected)) {
        showResult(Result::error(Message(QT_TRANSLATE_NOOP("Result", "Тип преобразования в файле работы не соответствует его расширению или варианту.")), ResultCategory::Progress));
        return false;
    }
    if (!confirmUnsavedChanges()) return false;
    Logger::instance().log(archiveResult);
    applyArchive(archivePath, variants);
    if (!setupWorkspace(*found)) return false;
    m_introCompleted = data.contains("introCompleted") ? data.value("introCompleted").toBool() :
        data.value("countsAccepted").toBool();
    auto sourceData = data.value("source").toObject();
    if (m_currentVariant.sourceKind() == AutomatonKind::Moore && !sourceData.contains("mooreSetup")) {
        const auto& counts = m_currentVariant.moore;
        sourceData.insert("mooreSetup", QJsonObject{{"states", data.value("stateCount").toInt(counts.states)},
            {"inputs", data.value("inputCount").toInt(counts.inputs)}, {"outputs", data.value("outputCount").toInt(counts.outputs)},
            {"accepted", data.value("countsAccepted").toBool()}});
    }
    m_sourceEditor->fromJson(sourceData);
    m_targetEditor->fromJson(data.value(QStringLiteral("target")).toObject());
    const QJsonObject report = data.value(QStringLiteral("report")).toObject();
    m_teacherName->setText(report.value(QStringLiteral("teacherName")).toString());
    m_studentName->setText(report.value(QStringLiteral("studentName")).toString());
    m_group->setText(report.value(QStringLiteral("group")).toString());
    m_recordBook->setText(report.value(QStringLiteral("recordBook")).toString());
    m_progressPath = progressPath;
    updateVariantSelection(m_currentVariant.number);
    updateWorkflowState();
    m_savedWorkState = workSnapshot();
    QTimer::singleShot(0, this, &MainWindow::fitStartPage);
    return true;
}

void MainWindow::createReport() {
    if (!m_targetEditor->completed()) return;
    const QList<QLineEdit*> required = {m_teacherName, m_studentName, m_group, m_recordBook};
    for (QLineEdit* edit : required) {
        if (edit->text().trimmed().isEmpty()) {
            showResult(Result::error(Message(QT_TRANSLATE_NOOP("Result", "Заполните все поля отчёта.")), ResultCategory::Report));
            edit->setFocus();
            return;
        }
    }
    QString path = QFileDialog::getSaveFileName(
        this, tr("Сохранить отчёт"),
        tr("report-variant-%1.html").arg(m_currentVariant.number),
        tr("Отчёт HTML (*.html)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".html"), Qt::CaseInsensitive)) path += QStringLiteral(".html");
    const QString html = reportHtml();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(html.toUtf8()) != html.toUtf8().size() || !file.commit()) {
        showResult(Result::error(Message(QT_TRANSLATE_NOOP("Result", "Не удалось сохранить отчёт: %1")).arg(file.errorString()), ResultCategory::Report).withDetails(path));
        return;
    }
    showResult(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Отчёт создан: %1")).arg(path), ResultCategory::Report));
}

QString MainWindow::reportHtml() const {
    QByteArray pngData;
    QBuffer imageBuffer(&pngData);
    imageBuffer.open(QIODevice::WriteOnly);
    QImage::fromData(m_currentVariant.diagram).save(&imageBuffer, "PNG");
    const QString image = QString::fromLatin1(pngData.toBase64());
    const QString mime = QStringLiteral("image/png");
    const auto caption = [](const char* source) { return QCoreApplication::translate("Report", source).toHtmlEscaped(); };
    const auto pair = [&](const char* label, const QString& value) {
        return "<p><b>" + caption(label) + "</b> " + value.toHtmlEscaped() + "</p>";
    };
    QString html = "<!doctype html><html lang='" + QString(TranslationManager::instance().locale() == "en" ? "en" : "ru") + "'><meta charset='utf-8'><title>" +
        caption(QT_TRANSLATE_NOOP("Report", "Отчёт M2MConvertor")) + "</title>" +
        "<style>body{font:16px sans-serif;max-width:900px;margin:40px auto}img{max-width:100%}</style><h1>" +
        caption(QT_TRANSLATE_NOOP("Report", "Преобразование эквивалентных автоматов")) + "</h1>";
    html += pair(QT_TRANSLATE_NOOP("Report", "Вариант:"), QString::number(m_currentVariant.number));
    html += pair(QT_TRANSLATE_NOOP("Report", "Направление:"), conversionDirectionName(m_currentVariant.direction));
    html += pair(QT_TRANSLATE_NOOP("Report", "Дата:"), QDateTime::currentDateTime().toString(Qt::ISODate));
    html += "<h2>" + caption(QT_TRANSLATE_NOOP("Report", "Участники")) + "</h2>";
    html += pair(QT_TRANSLATE_NOOP("Report", "Преподаватель:"), m_teacherName->text());
    html += pair(QT_TRANSLATE_NOOP("Report", "Студент:"), m_studentName->text());
    html += pair(QT_TRANSLATE_NOOP("Report", "Группа:"), m_group->text());
    html += pair(QT_TRANSLATE_NOOP("Report", "Зачётная книжка:"), m_recordBook->text());
    html += "<h2>" + caption(QT_TRANSLATE_NOOP("Report", "Результат")) + "</h2><p>" +
        automatonSummary(m_sourceEditor->acceptedAutomaton()).toHtmlEscaped() + "</p><p>" +
        automatonSummary(m_targetEditor->acceptedAutomaton()).toHtmlEscaped() + "</p><p>" +
        caption(QT_TRANSLATE_NOOP("Report", "Таблица, матрица и граф обоих автоматов приняты. Реакция каждого автомата из a0 совпадает с реакцией варианта. Проверена заданная последовательность входов; эквивалентность на всех возможных последовательностях этой проверкой не устанавливается.")) + "</p>";
    html += "<h2>" + UiText::text(UiText::presentationTitle(m_currentVariant.sourceKind() == AutomatonKind::Moore)).toHtmlEscaped() + "</h2><img src='data:" + mime + ";base64," + image + "'></html>";
    return html;
}

void MainWindow::showResult(const Result& result) {
    if (result.message().isEmpty()) return;
    Logger::instance().log(result);
    displayResult(result);
}
void MainWindow::displayResult(const Result& result) {
    const auto icon = result.level() == ResultLevel::Info ? QMessageBox::Information :
        (result.level() == ResultLevel::Warning || result.category() == ResultCategory::Validation)
            ? QMessageBox::Warning : QMessageBox::Critical;
    QMessageBox box(icon, tr("Внимание!"), result.text(), QMessageBox::Close, this);
    box.setObjectName("resultMessageDialog");
    box.setTextFormat(Qt::PlainText);
    box.button(QMessageBox::Close)->setText(tr("Закрыть"));
    box.exec();
}
void MainWindow::openLog() {
    if (!m_logWindow) {
        m_logWindow = new LogWindow(this);
        connect(m_logWindow, &LogWindow::notification, this, &MainWindow::displayResult);
    }
    m_logWindow->show(); m_logWindow->raise(); m_logWindow->activateWindow();
}
void MainWindow::changeEvent(QEvent* event) {
    QMainWindow::changeEvent(event);
    if (m_ready && event->type() == QEvent::LanguageChange) refreshTexts();
}
void MainWindow::refreshTexts() {
    if (!m_ready) return;
    UiText::refresh(this);
    m_russianAction->setChecked(TranslationManager::instance().locale() == "ru_RU");
    m_englishAction->setChecked(TranslationManager::instance().locale() == "en");
    updateVariantSelection(m_variantNumber->value());
    refreshArchiveInfo();
    refreshVariantInfo();
    if (m_hasVariant)
        m_variantTitle->setText(tr("Вариант №%1 — %2").arg(m_currentVariant.number).arg(conversionDirectionName(m_currentVariant.direction)));
    m_sourceEditor->retranslateTexts(); m_targetEditor->retranslateTexts();
}
