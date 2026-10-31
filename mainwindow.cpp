#include "infrastructure/logger.h"
#include "infrastructure/translationmanager.h"
#include "ui/logwindow.h"
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
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
#include "ui/widgets/actionbar.h"
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
#include <QKeySequence>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSaveFile>
#include <QScreen>
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
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(16);
    m_landingLogo = new QLabel;
    m_landingLogo->setObjectName("landingLogo");
    m_landingLogo->setAlignment(Qt::AlignCenter);
    m_landingLogo->setFixedHeight(88);
    m_landingLogo->setPixmap(AppIcons::applicationIcon().pixmap(72, 72));
    outer->addWidget(m_landingLogo);
    auto* title = new WrappingLabel;
    UiText::bind(title, "text", QT_TRANSLATE_NOOP("App", "Преобразование конечных автоматов Мили и Мура"));
    title->setObjectName("landingTitle");
    auto titleFont = title->font(); titleFont.setPointSize(17); titleFont.setBold(true);
    title->setFont(titleFont); title->setAlignment(Qt::AlignCenter);
    outer->addWidget(title);

    auto* content = new QWidget;
    content->setObjectName("landingContent");
    auto* cards = new QHBoxLayout(content);
    cards->setContentsMargins(0, 0, 0, 0); cards->setSpacing(20);
    auto* archiveGroup = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "1. База вариантов"));
    archiveGroup->setObjectName("archiveGroup");
    auto* archiveLayout = new QVBoxLayout(archiveGroup);
    auto* archiveBody = new QWidget;
    auto* archiveText = new QVBoxLayout(archiveBody);
    archiveText->setContentsMargins(0, 0, 0, 0);
    auto* archiveHint = new WrappingLabel;
    UiText::bind(archiveHint, "text", QT_TRANSLATE_NOOP("App", "Откройте архив .m2m"));
    archiveText->addWidget(archiveHint);
    m_archivePathLabel = new WrappingLabel;
    m_archivePathLabel->setObjectName("archiveSummary");
    m_archivePathLabel->setTextFormat(Qt::PlainText);
    m_archivePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    archiveText->addWidget(m_archivePathLabel);
    archiveText->addStretch();
    archiveLayout->addWidget(new WorkScrollArea(archiveBody), 1);
    auto* archiveButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Открыть архив вариантов…"));
    UiText::bind(archiveButton, "toolTip", QT_TRANSLATE_NOOP("App", "Открыть архив вариантов…"));
    archiveButton->setObjectName("openArchiveButton");
    archiveButton->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    auto* archiveActions = new QHBoxLayout;
    archiveActions->addWidget(archiveButton);
    archiveLayout->addLayout(archiveActions);

    auto* variantGroup = UiText::make<QGroupBox>(QT_TRANSLATE_NOOP("App", "2. Вариант работы"));
    variantGroup->setObjectName("variantGroup");
    auto* variantLayout = new QVBoxLayout(variantGroup);
    auto* variantBody = new QWidget;
    auto* variantText = new QVBoxLayout(variantBody);
    variantText->setContentsMargins(0, 0, 0, 0);
    auto* variantHint = new WrappingLabel;
    UiText::bind(variantHint, "text", QT_TRANSLATE_NOOP("App", "Выберите номер варианта."));
    variantText->addWidget(variantHint);
    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_variantCombo = new QComboBox;
    m_variantCombo->setObjectName("variantCombo"); m_variantCombo->setEnabled(false);
    m_landingDirection = new WrappingLabel;
    UiText::bind(m_landingDirection, "text", QT_TRANSLATE_NOOP("App", "—"));
    UiText::row(form, QT_TRANSLATE_NOOP("App", "Вариант:"), m_variantCombo);
    UiText::row(form, QT_TRANSLATE_NOOP("App", "Преобразование:"), m_landingDirection);
    variantText->addLayout(form); variantText->addStretch();
    variantLayout->addWidget(new WorkScrollArea(variantBody), 1);
    auto* beginButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Создать работу"));
    m_beginButton = beginButton;
    UiText::bind(m_beginButton, "toolTip", QT_TRANSLATE_NOOP("App", "Создать работу"));
    m_beginButton->setObjectName("beginWorkButton"); m_beginButton->setEnabled(false);
    m_beginButton->setDefault(true); m_beginButton->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    auto* open = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Открыть работу…"));
    UiText::bind(open, "toolTip", QT_TRANSLATE_NOOP("App", "Открыть работу…"));
    open->setObjectName("openProgressButton"); open->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    auto* variantActions = new QHBoxLayout;
    variantActions->addWidget(m_beginButton); variantActions->addStretch(); variantActions->addWidget(open);
    variantLayout->addLayout(variantActions);
    cards->addWidget(archiveGroup, 1); cards->addWidget(variantGroup, 1);
    outer->addWidget(content, 1);
    connect(archiveButton, &QPushButton::clicked, this, &MainWindow::chooseArchive);
    connect(m_variantCombo, &QComboBox::currentIndexChanged, this, &MainWindow::updateVariantSelection);
    connect(m_beginButton, &QPushButton::clicked, this, &MainWindow::beginWork);
    connect(open, &QPushButton::clicked, this, &MainWindow::openProgress);
    return m_landingPage;
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
    m_diagramLabel = new TaskImage;
    m_diagramLabel->setObjectName("startTaskImage");
    auto* explanation = new QWidget;
    auto* textLayout = new QVBoxLayout(explanation);
    textLayout->setContentsMargins(0, 0, 0, 0); textLayout->setSpacing(12);
    m_startVariantLabel = new WrappingLabel;
    m_startVariantLabel->setObjectName("startVariantInfo");
    auto font = m_startVariantLabel->font(); font.setBold(true); m_startVariantLabel->setFont(font);
    textLayout->addWidget(m_startVariantLabel);
    auto* instructions = new WrappingLabel;
    instructions->setObjectName("workInstructions");
    UiText::bind(instructions, "text", QT_TRANSLATE_NOOP("App", "Изучите изображение задания. Нажмите «Продолжить», чтобы перейти к таблице автомата Мура.\n\n"
        "В таблице Мура укажите количества состояний, входных и выходных сигналов и примите размеры. Заполните входы x0…x9, выходы y0…y9 и переходы между состояниями a0, a1, …; a0 всегда начальное. «-» обозначает отсутствие перехода или выхода.\n\n"
        "Принимайте представления по порядку: таблица, матрица A и граф Мура, затем таблица, матрица B и граф эквивалентного автомата Мили. Следующий этап откроется после правильного выполнения предыдущего.\n\n"
        "Каждое представление проверяется по реакции варианта из a0. Для Мура сравнивается выход до перехода, для Мили — выход на переходе. Совпадение конечной реакции не доказывает эквивалентность на всех входных последовательностях.\n\n"
        "Подсказку можно уменьшить или скрыть разделителем. Примеры ввода доступны при наведении на ячейки. Ctrl+S сохраняет работу; в конце заполните сведения об участниках и создайте отчёт."));
    textLayout->addWidget(instructions); textLayout->addStretch();
    m_startWorkScroll = new WorkScrollArea(explanation);
    m_startWorkScroll->setObjectName("startWorkArea");
    auto* work = new QWidget;
    auto* workLayout = new QVBoxLayout(work);
    workLayout->setContentsMargins(0, 0, 0, 0);
    workLayout->addWidget(m_startWorkScroll, 1);
    auto* next = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Продолжить"));
    next->setObjectName("continueWorkButton"); next->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    auto* actions = new ActionBar; actions->addWidget(next); workLayout->addWidget(actions);
    m_startSplitter = new HintSplitter(QT_TRANSLATE_NOOP("App", "Задание варианта"), m_diagramLabel, work,
                                      "startAssignment", Qt::Horizontal);
    m_startSplitter->setWorkTitle(QT_TRANSLATE_NOOP("App", "Пояснения к работе"));
    layout->addWidget(m_startSplitter, 1);
    connect(next, &QPushButton::clicked, this, &MainWindow::continueWork);
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
    refreshArchiveInfo();
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
    if (!confirmUnsavedChanges()) return;
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
    m_introCompleted = false;
    m_variantTitle->setText(tr("Вариант №%1 — %2")
        .arg(variant.number).arg(conversionDirectionName(variant.direction)));
    refreshVariantInfo();
    m_diagramLabel->setImageData(variant.diagram);
    m_notification->clearMessage();
    m_sourceEditor->setTask(variant, AutomatonKind::Moore);
    m_targetEditor->setTask(variant, AutomatonKind::Mealy);
    m_workflowTabs->setCurrentIndex(0);
    QTimer::singleShot(0, this, &MainWindow::fitStartPage);
    m_teacherName->clear();
    m_studentName->clear();
    m_group->clear(); m_recordBook->clear();
    m_saveAction->setEnabled(true);
    m_saveAsAction->setEnabled(true);
    updateWorkflowState();
    m_savedWorkState = workSnapshot();
    return true;
}

void MainWindow::fitStartPage() {
    if (m_workflowTabs->currentIndex() != 0) return;
    m_startSplitter->setHintExtent((m_startSplitter->width() - m_startSplitter->handleWidth()) / 2);
}

void MainWindow::continueWork() {
    if (!m_hasVariant) return;
    m_introCompleted = true;
    updateWorkflowState();
    m_workflowTabs->setCurrentIndex(1);
}

void MainWindow::refreshArchiveInfo() {
    m_archivePathLabel->setText(m_archivePath.isEmpty() ? tr("Архив не открыт") :
        tr("%1\nВариантов: %2").arg(QFileInfo(m_archivePath).fileName()).arg(m_variants.size()));
    m_archivePathLabel->setToolTip(m_archivePath);
}

void MainWindow::refreshVariantInfo() {
    m_startVariantLabel->setText(m_hasVariant ? tr("Вариант №%1\nПреобразование: %2")
        .arg(m_currentVariant.number).arg(conversionDirectionName(m_currentVariant.direction)) : QString());
}

void MainWindow::updateWorkflowState() {
    m_workflowTabs->setTabEnabled(1, m_introCompleted);
    const bool sourceComplete = m_introCompleted && m_sourceEditor->completed();
    m_workflowTabs->setTabEnabled(2, sourceComplete);
    const bool targetComplete = sourceComplete && m_targetEditor->completed();
    m_workflowTabs->setTabEnabled(3, targetComplete);
}

void MainWindow::returnToLanding() {
    if (!confirmUnsavedChanges()) return;
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
    if (saveAs || path.isEmpty()) {
        path = QFileDialog::getSaveFileName(
            this, tr("Сохранить учебную работу"),
            tr("variant-%1.m2mwork").arg(m_currentVariant.number),
            tr("Работа M2MConvertor (*.m2mwork)"));
        if (path.isEmpty()) return false;
        if (!path.endsWith(QStringLiteral(".m2mwork"), Qt::CaseInsensitive))
            path += QStringLiteral(".m2mwork");
    }
    const auto result = ProgressStore::write(path, progressData());
    showResult(result);
    if (result.ok()) {
        m_progressPath = path;
        m_savedWorkState = workSnapshot();
    }
    return result.ok();
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
    if (!confirmUnsavedChanges()) return false;
    Logger::instance().log(archiveResult);
    applyArchive(archivePath, variants);
    if (!setupWorkspace(*found)) return false;
    m_introCompleted = data.contains("introCompleted") ? data.value("introCompleted").toBool() :
        data.value("countsAccepted").toBool();
    auto sourceData = data.value("source").toObject();
    if (!sourceData.contains("mooreSetup")) {
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
    updateWorkflowState();
    m_savedWorkState = workSnapshot();
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
    if (m_ready && (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange))
        m_landingLogo->setPixmap(AppIcons::applicationIcon().pixmap(72, 72));
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
    refreshArchiveInfo();
    if (m_hasVariant) {
        m_variantTitle->setText(tr("Вариант №%1 — %2").arg(m_currentVariant.number).arg(conversionDirectionName(m_currentVariant.direction)));
        refreshVariantInfo();
    }
    m_sourceEditor->retranslateTexts(); m_targetEditor->retranslateTexts();
    if (m_lastResult) m_notification->updateMessageText(m_lastResult->text());
}
