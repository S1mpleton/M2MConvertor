#include "infrastructure/logger.h"
#include "infrastructure/translationmanager.h"
#include "ui/logwindow.h"
#include <QActionGroup>
#include <QEvent>
#include <QSignalBlocker>
#include "ui/uitext.h"
#include "mainwindow.h"

#include "infrastructure/progressstore.h"
#include "ui/automatoneditor.h"
#include "ui/widgets/feedbacklabel.h"
#include "ui/widgets/taskimage.h"
#include "ui/widgets/hintsplitter.h"
#include "ui/widgets/wrappinglabel.h"
#include <QShowEvent>
#include <QTimer>
#include "ui/widgets/workscrollarea.h"
#include "ui/workflowtabs.h"
#include "ui/appicons.h"

#include <QAction>
#include <QBuffer>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
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
    m_notification = new FeedbackLabel;
    m_notification->setObjectName("applicationNotification");
    auto* notificationArea = new QWidget;
    notificationArea->setObjectName("notificationArea");
    auto* notificationLayout = new QVBoxLayout(notificationArea);
    notificationLayout->setContentsMargins(10, 0, 10, 8);
    notificationLayout->addWidget(m_notification);
    centralLayout->addWidget(notificationArea);
    connect(m_notification, &FeedbackLabel::activeChanged, notificationArea, &QWidget::setVisible);
    notificationArea->hide();
    for (auto* editor : {m_sourceEditor, m_targetEditor})
        connect(editor, &AutomatonEditor::feedbackRequested, this, &MainWindow::showResult);
    connect(&TranslationManager::instance(), &TranslationManager::languageChanged, this, &MainWindow::refreshTexts);
    connect(m_notification, &FeedbackLabel::expired, this, [this] { m_lastResult.reset(); });
    m_ready = true;
    refreshTexts();
    Logger::instance().log(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Приложение запущено."))));
    layout()->activate();
    resize(minimumSizeHint().expandedTo(QSize(0, 0)));
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (m_initialSizeApplied) return;
    m_initialSizeApplied = true;
    QTimer::singleShot(0, this, [this] {
        layout()->activate();
        resize(minimumSizeHint().expandedTo(QSize(0, 0)));
    });
}

QWidget* MainWindow::createLandingPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(36, 28, 36, 32);
    outer->setSpacing(18);
    auto* title = new WrappingLabel;
    UiText::bind(title, "text", QT_TRANSLATE_NOOP("App", "Преобразование конечных автоматов Мили и Мура"));
    title->setObjectName(QStringLiteral("landingTitle"));
    QFont titleFont = title->font();
    titleFont.setPointSize(17);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    auto* explanation = new WrappingLabel;
    UiText::bind(explanation, "text", QT_TRANSLATE_NOOP("App", "Откройте зашифрованную базу .m2m с реакциями, созданную в M2MTools, и выберите вариант. "
        "Направление преобразования определяется самим вариантом."));
    explanation->setWordWrap(true);
    explanation->setAlignment(Qt::AlignCenter);
    outer->addWidget(title);
    outer->addWidget(explanation);

    auto* content = new QWidget;
    content->setObjectName(QStringLiteral("landingContent"));
    auto* contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 12, 0, 12);
    contentLayout->setSpacing(24);

    auto* archiveGroup = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "1. База вариантов"));
    auto* archiveLayout = new QVBoxLayout(archiveGroup);
    archiveLayout->setContentsMargins(20, 24, 20, 20);
    archiveLayout->setSpacing(14);
    auto* archiveHint = new WrappingLabel;
    UiText::bind(archiveHint, "text", QT_TRANSLATE_NOOP("App", "Выберите файл .m2m с реакциями, экспортированный из M2MTools. "
        "Приложение проверит целостность архива перед показом вариантов."));
    archiveHint->setWordWrap(true);
    archiveLayout->addWidget(archiveHint);
    m_archivePathLabel = new WrappingLabel;
    UiText::bind(m_archivePathLabel, "text", QT_TRANSLATE_NOOP("App", "BazaVar не выбран"));
    m_archivePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_archivePathLabel->setWordWrap(true);
    auto* archiveButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Открыть BazaVar…"));
    archiveButton->setObjectName(QStringLiteral("openArchiveButton"));
    archiveButton->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    archiveLayout->addWidget(m_archivePathLabel);
    archiveLayout->addWidget(archiveButton, 0, Qt::AlignLeft);
    archiveLayout->addStretch();

    auto* variantGroup = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "2. Вариант работы"));
    auto* variantLayout = new QVBoxLayout(variantGroup);
    variantLayout->setContentsMargins(20, 24, 20, 20);
    variantLayout->setSpacing(14);
    auto* variantHint = new WrappingLabel;
    UiText::bind(variantHint, "text", QT_TRANSLATE_NOOP("App", "Выберите номер варианта. Направление преобразования уже задано в базе "
        "и будет выбрано автоматически."));
    variantHint->setWordWrap(true);
    variantLayout->addWidget(variantHint);
    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_variantCombo = new QComboBox;
    m_variantCombo->setObjectName("variantCombo");
    m_variantCombo->setEnabled(false);
    m_landingDirection = new WrappingLabel;
    UiText::bind(m_landingDirection, "text", QT_TRANSLATE_NOOP("App", "—"));
    m_landingDirection->setWordWrap(true);
    UiText::row(form, QT_TRANSLATE_NOOP("App", "Вариант:"), m_variantCombo);
    UiText::row(form, QT_TRANSLATE_NOOP("App", "Преобразование:"), m_landingDirection);
    variantLayout->addLayout(form);
    variantLayout->addStretch();

    contentLayout->addWidget(archiveGroup, 1);
    contentLayout->addWidget(variantGroup, 1);
    outer->addWidget(content, 1);

    auto* actionHint = new WrappingLabel;
    UiText::bind(actionHint, "text", QT_TRANSLATE_NOOP("App", "Создайте новую работу для выбранного варианта или продолжите ранее сохранённую."));
    actionHint->setAlignment(Qt::AlignCenter);
    actionHint->setWordWrap(true);
    outer->addWidget(actionHint);
    auto* actions = new QHBoxLayout;
    m_beginButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Создать работу"));
    m_beginButton->setObjectName(QStringLiteral("beginWorkButton"));
    m_beginButton->setEnabled(false);
    m_beginButton->setDefault(true);
    m_beginButton->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    auto* openProgressButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Открыть сохранённую работу…"));
    openProgressButton->setObjectName(QStringLiteral("openProgressButton"));
    openProgressButton->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    actions->addStretch();
    actions->addWidget(m_beginButton);
    actions->addWidget(openProgressButton);
    actions->addStretch();
    outer->addLayout(actions);
    connect(archiveButton, &QPushButton::clicked, this, &MainWindow::chooseArchive);
    connect(m_variantCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::updateVariantSelection);
    connect(m_beginButton, &QPushButton::clicked, this, &MainWindow::beginWork);
    connect(openProgressButton, &QPushButton::clicked, this, &MainWindow::openProgress);
    return page;
}

QWidget* MainWindow::createWorkspacePage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 10, 10, 10);
    auto* top = new QHBoxLayout;
    auto* back = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "К выбору варианта"));
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
    UiText::tab(m_workflowTabs, createStartPage(), QT_TRANSLATE_NOOP("App", "Начало работы"));
    m_sourceEditor = new AutomatonEditor;
    m_sourceEditor->setObjectName("sourceEditor");
    m_targetEditor = new AutomatonEditor;
    m_targetEditor->setObjectName("targetEditor");
    UiText::tab(m_workflowTabs, m_sourceEditor, QT_TRANSLATE_NOOP("App", "Автомат Мура"));
    UiText::tab(m_workflowTabs, m_targetEditor, QT_TRANSLATE_NOOP("App", "Автомат Мили"));
    UiText::tab(m_workflowTabs, createReportPage(), QT_TRANSLATE_NOOP("App", "Отчёт"));
    m_workflowTabs->setTabEnabled(1, false);
    m_workflowTabs->setTabEnabled(2, false);
    m_workflowTabs->setTabEnabled(3, false);
    layout->addWidget(m_workflowTabs, 1);
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
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);
    m_directionLabel = new QLabel;
    QFont directionFont = m_directionLabel->font();
    directionFont.setPointSize(15);
    directionFont.setBold(true);
    m_directionLabel->setFont(directionFont);
    m_directionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(m_directionLabel);

    m_diagramLabel = new TaskImage;
    m_diagramLabel->setObjectName("startTaskImage");

    auto* workPanel = new QWidget;
    auto* right = new QVBoxLayout(workPanel);
    right->setContentsMargins(0, 0, 0, 0);
    right->setSpacing(14);
    auto* instructionsGroup = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "Порядок работы"));
    auto* instructionsLayout = new QVBoxLayout(instructionsGroup);
    auto* instructions = new WrappingLabel;
    UiText::bind(instructions, "text", QT_TRANSLATE_NOOP("App", "Изучите задание на рисунке и укажите размер исходного автомата. После правильного "
        "ответа откроется последовательная работа с таблицей, матрицей и графом."));
    instructions->setWordWrap(true);
    instructionsLayout->addWidget(instructions);
    right->addWidget(instructionsGroup);
    auto* counts = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "Проверьте структуру исходного автомата"));
    auto* countLayout = new QFormLayout(counts);
    countLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    countLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    m_stateCount = new QSpinBox;
    m_inputCount = new QSpinBox;
    m_outputCount = new QSpinBox;
    m_stateCount->setRange(1, 999999);
    m_inputCount->setRange(1, 10);
    m_outputCount->setRange(1, 10);
    UiText::row(countLayout, QT_TRANSLATE_NOOP("App", "Количество состояний:"), m_stateCount);
    UiText::row(countLayout, QT_TRANSLATE_NOOP("App", "Количество входных сигналов:"), m_inputCount);
    UiText::row(countLayout, QT_TRANSLATE_NOOP("App", "Количество выходных сигналов:"), m_outputCount);
    auto* accept = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять"));
    accept->setObjectName(QStringLiteral("acceptCountsButton"));
    accept->setDefault(true);
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    countLayout->addRow(accept);
    right->addWidget(counts);
    right->addStretch();
    auto* workScroll = new WorkScrollArea(workPanel);
    auto* splitter = new HintSplitter(QT_TRANSLATE_NOOP("App", "Задание варианта"), m_diagramLabel, workScroll,
                                      "startAssignment", Qt::Horizontal);
    layout->addWidget(splitter, 1);
    connect(accept, &QPushButton::clicked, this, &MainWindow::acceptCounts);
    auto invalidate = [this] {
        if (!m_countsAccepted) return;
        m_countsAccepted = false;
        m_sourceEditor->resetEditor();
        m_targetEditor->resetEditor();
        updateWorkflowState();
    };
    connect(m_stateCount, &QSpinBox::valueChanged, this, invalidate);
    connect(m_inputCount, &QSpinBox::valueChanged, this, invalidate);
    connect(m_outputCount, &QSpinBox::valueChanged, this, invalidate);
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
    layout->addWidget(create, 0, Qt::AlignCenter);
    layout->addStretch();
    connect(create, &QPushButton::clicked, this, &MainWindow::createReport);
    return page;
}

void MainWindow::createMenus() {
    AppIcons::configureMenuBar(menuBar());
    auto* file = UiText::menu(menuBar(), AppIcons::icon(AppIcons::Kind::File), QT_TRANSLATE_NOOP("App", "Файл"));
    auto* newWork = UiText::action(file, AppIcons::icon(AppIcons::Kind::New), QT_TRANSLATE_NOOP("App", "Выбрать другой вариант"));
    auto* open = UiText::action(file, AppIcons::icon(AppIcons::Kind::Open), QT_TRANSLATE_NOOP("App", "Открыть работу…"));
    m_saveAction = UiText::action(file, AppIcons::icon(AppIcons::Kind::Save), QT_TRANSLATE_NOOP("App", "Сохранить работу"));
    m_saveAsAction = UiText::action(file, AppIcons::icon(AppIcons::Kind::Save), QT_TRANSLATE_NOOP("App", "Сохранить работу как…"));
    file->addSeparator();
    auto* quit = UiText::action(file, AppIcons::icon(AppIcons::Kind::Exit), QT_TRANSLATE_NOOP("App", "Выход"));
    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);
    auto* settings = UiText::menu(menuBar(), AppIcons::icon(AppIcons::Kind::Settings), QT_TRANSLATE_NOOP("App", "Настройки"));
    settings->setObjectName("settingsMenu");
    auto* logAction = UiText::action(settings, AppIcons::icon(AppIcons::Kind::Log), QT_TRANSLATE_NOOP("App", "Журнал событий…"));
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
        this, tr("Открыть BazaVar"), QString(), tr("BazaVar M2MTools (*.m2m);;Все файлы (*)"));
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
    applyArchive(path, variants);
    showResult(result);
    return true;
}

void MainWindow::applyArchive(const QString& path, const QList<TrainingVariant>& variants) {
    m_archivePath = QFileInfo(path).absoluteFilePath();
    m_variants = variants;
    m_archivePathLabel->setText(QFileInfo(path).fileName());
    m_archivePathLabel->setToolTip(m_archivePath);
    m_variantCombo->blockSignals(true);
    m_variantCombo->clear();
    int firstAvailable = -1;
    for (const TrainingVariant& variant : variants) {
        const int row = m_variantCombo->count();
        m_variantCombo->addItem(variant.available() ? tr("Вариант №%1").arg(variant.number)
            : tr("Вариант №%1 — Мили → Мура (в разработке)").arg(variant.number), variant.number);
        if (!variant.available()) {
            if (auto* model = qobject_cast<QStandardItemModel*>(m_variantCombo->model()))
                model->item(row)->setEnabled(false);
        } else if (firstAvailable < 0) firstAvailable = row;
    }
    m_variantCombo->setCurrentIndex(firstAvailable);
    m_variantCombo->setEnabled(true);
    m_variantCombo->blockSignals(false);
    updateVariantSelection(m_variantCombo->currentIndex());
}

void MainWindow::updateVariantSelection(int index) {
    const bool valid = index >= 0 && index < m_variants.size();
    m_beginButton->setEnabled(valid && m_variants[index].available());
    m_landingDirection->setText(valid
        ? (m_variants[index].available() ? conversionDirectionName(m_variants[index].direction)
                                        : tr("Мили → Мура — в разработке"))
        : (m_variants.isEmpty() ? QStringLiteral("—")
                               : tr("В базе нет доступных вариантов Мура → Мили.")));
}

void MainWindow::beginWork() {
    const int index = m_variantCombo->currentIndex();
    if (index < 0 || index >= m_variants.size()) return;
    if (!setupWorkspace(m_variants[index])) return;
    m_progressPath.clear();
    m_pages->setCurrentIndex(kWorkspacePage);
    showResult(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Начата работа с вариантом №%1.")).arg(m_currentVariant.number), ResultCategory::Progress));
}

bool MainWindow::setupWorkspace(const TrainingVariant& variant) {
    if (!variant.available()) {
        UiText::bind(m_landingDirection, "text", QT_TRANSLATE_NOOP("App", "Мили → Мура — в разработке. Выберите вариант Мура → Мили."));
        return false;
    }
    m_currentVariant = variant;
    m_hasVariant = true;
    m_countsAccepted = false;
    m_variantTitle->setText(tr("Вариант №%1 — %2")
        .arg(variant.number).arg(conversionDirectionName(variant.direction)));
    m_directionLabel->setText(conversionDirectionName(variant.direction));
    m_diagramLabel->setImageData(variant.diagram);
    m_notification->clearMessage();
    m_stateCount->setValue(1);
    m_inputCount->setValue(1);
    m_outputCount->setValue(1);
    m_sourceEditor->setTask(variant, AutomatonKind::Moore);
    m_targetEditor->setTask(variant, AutomatonKind::Mealy);
    m_workflowTabs->setCurrentIndex(0);
    m_teacherName->clear();
    m_studentName->clear();
    m_group->clear(); m_recordBook->clear();
    m_saveAction->setEnabled(true);
    m_saveAsAction->setEnabled(true);
    updateWorkflowState();
    return true;
}

void MainWindow::acceptCounts() {
    if (!m_hasVariant) return;
    const auto& source = m_currentVariant.moore;
    const bool correct = m_stateCount->value() == source.states &&
                         m_inputCount->value() == source.inputs &&
                         m_outputCount->value() == source.outputs;
    if (!correct) {
        m_countsAccepted = false;
        setFeedback(Message(QT_TRANSLATE_NOOP("Result", "Неверно. Введённые данные не соответствуют варианту.")), false);
        updateWorkflowState();
        return;
    }
    m_countsAccepted = true;
    setFeedback(Message(QT_TRANSLATE_NOOP("Result", "Верно. Введённые данные соответствуют варианту.")), true);
    updateWorkflowState();
    m_workflowTabs->setCurrentIndex(1);
}

void MainWindow::updateWorkflowState() {
    m_workflowTabs->setTabEnabled(1, m_countsAccepted);
    const bool sourceComplete = m_countsAccepted && m_sourceEditor->completed();
    m_workflowTabs->setTabEnabled(2, sourceComplete);
    const bool targetComplete = sourceComplete && m_targetEditor->completed();
    m_workflowTabs->setTabEnabled(3, targetComplete);
}

void MainWindow::returnToLanding() {
    if (m_pages->currentIndex() == kWorkspacePage && m_hasVariant) {
        const auto answer = QMessageBox::question(
            this, tr("Завершить текущую работу?"),
            tr("Несохранённые данные текущей работы будут потеряны. Продолжить?"));
        if (answer != QMessageBox::Yes) return;
    }
    Logger::instance().log(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Возврат к выбору варианта.")), ResultCategory::Progress));
    m_pages->setCurrentIndex(kLandingPage);
    m_hasVariant = false;
    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);
}

QJsonObject MainWindow::progressData() const {
    QJsonObject data;
    data.insert(QStringLiteral("archivePath"), m_archivePath);
    data.insert(QStringLiteral("variantNumber"), m_currentVariant.number);
    data.insert(QStringLiteral("countsAccepted"), m_countsAccepted);
    data.insert(QStringLiteral("stateCount"), m_stateCount->value());
    data.insert(QStringLiteral("inputCount"), m_inputCount->value());
    data.insert(QStringLiteral("outputCount"), m_outputCount->value());
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

void MainWindow::saveProgress(bool saveAs) {
    if (!m_hasVariant) return;
    QString path = m_progressPath;
    if (saveAs || path.isEmpty()) {
        path = QFileDialog::getSaveFileName(
            this, tr("Сохранить учебную работу"),
            tr("variant-%1.m2mwork").arg(m_currentVariant.number),
            tr("Работа M2MConvertor (*.m2mwork)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(QStringLiteral(".m2mwork"), Qt::CaseInsensitive))
            path += QStringLiteral(".m2mwork");
    }
    const auto result = ProgressStore::write(path, progressData());
    showResult(result);
    if (result.ok()) m_progressPath = path;

}

void MainWindow::openProgress() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Открыть учебную работу"), QString(),
        tr("Работа M2MConvertor (*.m2mwork);;Все файлы (*)"));
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
            tr("BazaVar M2MTools (*.m2m);;Все файлы (*)"));
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
    if (!found->available()) {
        showResult(Result::warning(Message(QT_TRANSLATE_NOOP("Result", "Открытие работ Мили → Мура пока недоступно.")), ResultCategory::Progress));
        return false;
    }
    Logger::instance().log(archiveResult);
    applyArchive(archivePath, variants);
    if (!setupWorkspace(*found)) return false;
    m_stateCount->setValue(data.value(QStringLiteral("stateCount")).toInt(1));
    m_inputCount->setValue(data.value(QStringLiteral("inputCount")).toInt(1));
    m_outputCount->setValue(data.value(QStringLiteral("outputCount")).toInt(1));
    m_countsAccepted = data.value(QStringLiteral("countsAccepted")).toBool() &&
        m_stateCount->value() == m_currentVariant.moore.states &&
        m_inputCount->value() == m_currentVariant.moore.inputs &&
        m_outputCount->value() == m_currentVariant.moore.outputs;
    m_sourceEditor->fromJson(data.value(QStringLiteral("source")).toObject());
    m_targetEditor->fromJson(data.value(QStringLiteral("target")).toObject());
    const QJsonObject report = data.value(QStringLiteral("report")).toObject();
    m_teacherName->setText(report.value(QStringLiteral("teacherName")).toString());
    m_studentName->setText(report.value(QStringLiteral("studentName")).toString());
    m_group->setText(report.value(QStringLiteral("group")).toString());
    m_recordBook->setText(report.value(QStringLiteral("recordBook")).toString());
    m_progressPath = progressPath;
    updateWorkflowState();
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
    html += "<h2>" + caption(QT_TRANSLATE_NOOP("Report", "Задание")) + "</h2><img src='data:" + mime + ";base64," + image + "'></html>";
    return html;
}

void MainWindow::setFeedback(const Message& text, bool success) {
    showResult(success ? Result::success(text, ResultCategory::Validation) : Result::error(text, ResultCategory::Validation));
}

void MainWindow::showResult(const Result& result) {
    if (result.message().isEmpty()) return;
    Logger::instance().log(result);
    displayResult(result);
}
void MainWindow::displayResult(const Result& result) {
    m_lastResult = result;
    m_notification->showMessage(result.text(), result.level() == ResultLevel::Error ? UiFeedbackLevel::Error :
        result.level() == ResultLevel::Warning ? UiFeedbackLevel::Warning : UiFeedbackLevel::Success);
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
    const QSignalBlocker blocker(m_variantCombo);
    for (int index = 0; index < m_variants.size(); ++index)
        m_variantCombo->setItemText(index, m_variants[index].available()
            ? tr("Вариант №%1").arg(m_variants[index].number)
            : tr("Вариант №%1 — Мили → Мура (в разработке)").arg(m_variants[index].number));
    updateVariantSelection(m_variantCombo->currentIndex());
    if (!m_archivePath.isEmpty()) m_archivePathLabel->setText(QFileInfo(m_archivePath).fileName());
    if (m_hasVariant) {
        m_variantTitle->setText(tr("Вариант №%1 — %2").arg(m_currentVariant.number).arg(conversionDirectionName(m_currentVariant.direction)));
        m_directionLabel->setText(conversionDirectionName(m_currentVariant.direction));
    }
    m_sourceEditor->retranslateTexts(); m_targetEditor->retranslateTexts();
    if (m_lastResult) m_notification->updateMessageText(m_lastResult->text());
}
