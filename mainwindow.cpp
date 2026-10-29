#include "mainwindow.h"

#include "infrastructure/progressstore.h"
#include "ui/automatoneditor.h"
#include "ui/widgets/feedbacklabel.h"
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
#include <QStatusBar>
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
    return QStringLiteral("%1: %2 состояний, %3 входов, %4 выходов, %5 переходов")
        .arg(automatonKindName(automaton.kind))
        .arg(automaton.states.size()).arg(automaton.inputs.size())
        .arg(automaton.outputs.size()).arg(automaton.transitions.size());
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("M2MConvertor — эквивалентные автоматы"));
    resize(1200, 820);
    setMinimumSize(900, 640);
    m_pages = new QStackedWidget;
    m_pages->addWidget(createLandingPage());
    m_pages->addWidget(createWorkspacePage());
    setCentralWidget(m_pages);
    createMenus();
    m_notification = new FeedbackLabel;
    m_notification->setObjectName("applicationNotification");
    statusBar()->addWidget(m_notification, 1);
}

QWidget* MainWindow::createLandingPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(36, 28, 36, 32);
    outer->setSpacing(18);
    auto* title = new QLabel(tr("Преобразование конечных автоматов Мили и Мура"));
    title->setObjectName(QStringLiteral("landingTitle"));
    QFont titleFont = title->font();
    titleFont.setPointSize(17);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    auto* explanation = new QLabel(tr(
        "Откройте зашифрованную базу .m2m с реакциями, созданную в M2MTools, и выберите вариант. "
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

    auto* archiveGroup = new QGroupBox(tr("1. База вариантов"));
    auto* archiveLayout = new QVBoxLayout(archiveGroup);
    archiveLayout->setContentsMargins(20, 24, 20, 20);
    archiveLayout->setSpacing(14);
    auto* archiveHint = new QLabel(tr(
        "Выберите файл .m2m с реакциями, экспортированный из M2MTools. "
        "Приложение проверит целостность архива перед показом вариантов."));
    archiveHint->setWordWrap(true);
    archiveLayout->addWidget(archiveHint);
    auto* archiveRow = new QHBoxLayout;
    m_archivePathLabel = new QLabel(tr("BazaVar не выбран"));
    m_archivePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_archivePathLabel->setWordWrap(true);
    auto* archiveButton = new QPushButton(tr("Открыть BazaVar…"));
    archiveButton->setObjectName(QStringLiteral("openArchiveButton"));
    archiveButton->setIcon(AppIcons::icon(AppIcons::Kind::Open));
    archiveRow->addWidget(m_archivePathLabel, 1);
    archiveRow->addWidget(archiveButton);
    archiveLayout->addLayout(archiveRow);
    archiveLayout->addStretch();

    auto* variantGroup = new QGroupBox(tr("2. Вариант работы"));
    auto* variantLayout = new QVBoxLayout(variantGroup);
    variantLayout->setContentsMargins(20, 24, 20, 20);
    variantLayout->setSpacing(14);
    auto* variantHint = new QLabel(tr(
        "Выберите номер варианта. Направление преобразования уже задано в базе "
        "и будет выбрано автоматически."));
    variantHint->setWordWrap(true);
    variantLayout->addWidget(variantHint);
    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_variantCombo = new QComboBox;
    m_variantCombo->setObjectName("variantCombo");
    m_variantCombo->setEnabled(false);
    m_landingDirection = new QLabel(tr("—"));
    m_landingDirection->setWordWrap(true);
    form->addRow(tr("Вариант:"), m_variantCombo);
    form->addRow(tr("Преобразование:"), m_landingDirection);
    variantLayout->addLayout(form);
    variantLayout->addStretch();

    contentLayout->addWidget(archiveGroup, 1);
    contentLayout->addWidget(variantGroup, 1);
    outer->addWidget(content, 1);

    auto* actionHint = new QLabel(tr(
        "Создайте новую работу для выбранного варианта или продолжите ранее сохранённую."));
    actionHint->setAlignment(Qt::AlignCenter);
    actionHint->setWordWrap(true);
    outer->addWidget(actionHint);
    auto* actions = new QHBoxLayout;
    m_beginButton = new QPushButton(tr("Создать работу"));
    m_beginButton->setObjectName(QStringLiteral("beginWorkButton"));
    m_beginButton->setEnabled(false);
    m_beginButton->setDefault(true);
    m_beginButton->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    auto* openProgressButton = new QPushButton(tr("Открыть сохранённую работу…"));
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
    auto* top = new QHBoxLayout;
    auto* back = new QPushButton(tr("К выбору варианта"));
    back->setIcon(AppIcons::icon(AppIcons::Kind::Back));
    m_variantTitle = new QLabel;
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
    m_workflowTabs->addTab(createStartPage(), tr("Начало работы"));
    m_sourceEditor = new AutomatonEditor;
    m_sourceEditor->setObjectName("sourceEditor");
    m_targetEditor = new AutomatonEditor;
    m_targetEditor->setObjectName("targetEditor");
    m_workflowTabs->addTab(m_sourceEditor, tr("Автомат Мура"));
    m_workflowTabs->addTab(m_targetEditor, tr("Автомат Мили"));
    m_workflowTabs->addTab(createReportPage(), tr("Отчёт"));
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

    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setChildrenCollapsible(false);
    auto* assignmentGroup = new QGroupBox(tr("Задание варианта"));
    auto* assignmentLayout = new QVBoxLayout(assignmentGroup);
    m_diagramLabel = new QLabel(tr("В варианте отсутствует изображение"));
    m_diagramLabel->setAlignment(Qt::AlignCenter);
    m_diagramLabel->setMinimumSize(360, 300);
    m_diagramLabel->setFrameShape(QFrame::StyledPanel);
    m_diagramLabel->setFrameShadow(QFrame::Sunken);
    assignmentLayout->addWidget(m_diagramLabel, 1);

    auto* workPanel = new QWidget;
    auto* right = new QVBoxLayout(workPanel);
    right->setContentsMargins(8, 0, 0, 0);
    right->setSpacing(14);
    auto* instructionsGroup = new QGroupBox(tr("Порядок работы"));
    auto* instructionsLayout = new QVBoxLayout(instructionsGroup);
    auto* instructions = new QLabel(tr(
        "Изучите задание на рисунке и укажите размер исходного автомата. После правильного "
        "ответа откроется последовательная работа с таблицей, матрицей и графом."));
    instructions->setWordWrap(true);
    instructionsLayout->addWidget(instructions);
    right->addWidget(instructionsGroup);
    auto* counts = new QGroupBox(tr("Проверьте структуру исходного автомата"));
    auto* countLayout = new QFormLayout(counts);
    countLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_stateCount = new QSpinBox;
    m_inputCount = new QSpinBox;
    m_outputCount = new QSpinBox;
    m_stateCount->setRange(1, 999999);
    m_inputCount->setRange(1, 10);
    m_outputCount->setRange(1, 10);
    countLayout->addRow(tr("Количество состояний:"), m_stateCount);
    countLayout->addRow(tr("Количество входных сигналов:"), m_inputCount);
    countLayout->addRow(tr("Количество выходных сигналов:"), m_outputCount);
    auto* accept = new QPushButton(tr("Принять"));
    accept->setObjectName(QStringLiteral("acceptCountsButton"));
    accept->setDefault(true);
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    m_startFeedback = new FeedbackLabel;
    m_startFeedback->setWordWrap(true);
    countLayout->addRow(accept);
    countLayout->addRow(m_startFeedback);
    right->addWidget(counts);
    right->addStretch();
    splitter->addWidget(assignmentGroup);
    splitter->addWidget(workPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
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
    auto* teacher = new QGroupBox(tr("Преподаватель"));
    auto* teacherForm = new QFormLayout(teacher);
    m_teacherName = new QLineEdit;
    m_teacherId = new QLineEdit;
    teacherForm->addRow(tr("Ф. И. О. преподавателя:"), m_teacherName);
    teacherForm->addRow(tr("Идентификатор:"), m_teacherId);
    auto* student = new QGroupBox(tr("Студент"));
    auto* studentForm = new QFormLayout(student);
    m_studentName = new QLineEdit;
    m_studentId = new QLineEdit;
    m_group = new QLineEdit;
    m_recordBook = new QLineEdit;
    studentForm->addRow(tr("Ф. И. О. студента:"), m_studentName);
    studentForm->addRow(tr("Идентификатор:"), m_studentId);
    studentForm->addRow(tr("Группа:"), m_group);
    studentForm->addRow(tr("Номер зачётной книжки:"), m_recordBook);
    auto* create = new QPushButton(tr("Создать отчёт HTML…"));
    create->setObjectName(QStringLiteral("createReportButton"));
    create->setIcon(AppIcons::icon(AppIcons::Kind::Save));
    m_reportFeedback = new FeedbackLabel;
    m_reportFeedback->setWordWrap(true);
    layout->addWidget(teacher);
    layout->addWidget(student);
    layout->addWidget(create, 0, Qt::AlignCenter);
    layout->addWidget(m_reportFeedback);
    layout->addStretch();
    connect(create, &QPushButton::clicked, this, &MainWindow::createReport);
    return page;
}

void MainWindow::createMenus() {
    AppIcons::configureMenuBar(menuBar());
    auto* file = menuBar()->addMenu(AppIcons::icon(AppIcons::Kind::File), tr("Файл"));
    auto* newWork = file->addAction(AppIcons::icon(AppIcons::Kind::New),
                                    tr("Выбрать другой вариант"));
    auto* open = file->addAction(AppIcons::icon(AppIcons::Kind::Open),
                                 tr("Открыть работу…"));
    m_saveAction = file->addAction(AppIcons::icon(AppIcons::Kind::Save),
                                   tr("Сохранить работу"));
    m_saveAsAction = file->addAction(AppIcons::icon(AppIcons::Kind::Save),
                                     tr("Сохранить работу как…"));
    file->addSeparator();
    auto* quit = file->addAction(AppIcons::icon(AppIcons::Kind::Exit), tr("Выход"));
    m_saveAction->setEnabled(false);
    m_saveAsAction->setEnabled(false);
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
    QString error;
    if (!VariantArchive::load(path, &variants, &error)) {
        if (showErrors) m_notification->showMessage(error, UiFeedbackLevel::Error);
        return false;
    }
    if (variants.isEmpty()) {
        if (showErrors) m_notification->showMessage(tr("В BazaVar нет вариантов."), UiFeedbackLevel::Warning);
        return false;
    }
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
    return true;
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
}

bool MainWindow::setupWorkspace(const TrainingVariant& variant) {
    if (!variant.available()) {
        m_landingDirection->setText(tr("Мили → Мура — в разработке. Выберите вариант Мура → Мили."));
        return false;
    }
    m_currentVariant = variant;
    m_hasVariant = true;
    m_countsAccepted = false;
    m_variantTitle->setText(tr("Вариант №%1 — %2")
        .arg(variant.number).arg(conversionDirectionName(variant.direction)));
    m_directionLabel->setText(conversionDirectionName(variant.direction));
    QPixmap diagram;
    diagram.loadFromData(variant.diagram);
    if (!diagram.isNull())
        m_diagramLabel->setPixmap(diagram.scaled(m_diagramLabel->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
    else {
        m_diagramLabel->setPixmap(QPixmap());
        m_diagramLabel->setText(tr("Изображение задания отсутствует"));
    }
    m_startFeedback->clearMessage();
    m_stateCount->setValue(1);
    m_inputCount->setValue(1);
    m_outputCount->setValue(1);
    m_sourceEditor->setTask(variant, AutomatonKind::Moore);
    m_targetEditor->setTask(variant, AutomatonKind::Mealy);
    m_workflowTabs->setCurrentIndex(0);
    m_teacherName->clear(); m_teacherId->clear();
    m_studentName->clear(); m_studentId->clear();
    m_group->clear(); m_recordBook->clear();
    m_reportFeedback->clearMessage();
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
        setFeedback(m_startFeedback,
                    tr("Неверно. Введённые данные не соответствуют варианту."), false);
        updateWorkflowState();
        return;
    }
    m_countsAccepted = true;
    setFeedback(m_startFeedback,
                tr("Верно. Введённые данные соответствуют варианту."), true);
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
    report.insert(QStringLiteral("teacherId"), m_teacherId->text());
    report.insert(QStringLiteral("studentName"), m_studentName->text());
    report.insert(QStringLiteral("studentId"), m_studentId->text());
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
    QString error;
    if (!ProgressStore::save(path, progressData(), &error)) {
        m_notification->showMessage(error, UiFeedbackLevel::Error);
        return;
    }
    m_progressPath = path;
    m_notification->showMessage(tr("Работа сохранена: %1").arg(QFileInfo(path).fileName()), UiFeedbackLevel::Success);
}

void MainWindow::openProgress() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Открыть учебную работу"), QString(),
        tr("Работа M2MConvertor (*.m2mwork);;Все файлы (*)"));
    if (path.isEmpty()) return;
    QJsonObject data;
    QString error;
    if (!ProgressStore::load(path, &data, &error)) {
        m_notification->showMessage(error, UiFeedbackLevel::Error);
        return;
    }
    if (!restoreProgress(data, path)) return;
    m_pages->setCurrentIndex(kWorkspacePage);
}

bool MainWindow::restoreProgress(const QJsonObject& data, const QString& progressPath) {
    QString archivePath = data.value(QStringLiteral("archivePath")).toString();
    QList<TrainingVariant> variants;
    QString error;
    if (!VariantArchive::load(archivePath, &variants, &error)) {
        archivePath = QFileDialog::getOpenFileName(
            this, tr("Укажите BazaVar для сохранённой работы"), QString(),
            tr("BazaVar M2MTools (*.m2m);;Все файлы (*)"));
        if (archivePath.isEmpty()) return false;
        if (!VariantArchive::load(archivePath, &variants, &error)) {
            m_notification->showMessage(error, UiFeedbackLevel::Error);
            return false;
        }
    }
    const int variantNumber = data.value(QStringLiteral("variantNumber")).toInt();
    auto found = std::find_if(variants.cbegin(), variants.cend(),
        [variantNumber](const TrainingVariant& variant) { return variant.number == variantNumber; });
    if (found == variants.cend()) {
        m_notification->showMessage(tr("В текущем BazaVar нет варианта №%1.").arg(variantNumber), UiFeedbackLevel::Error);
        return false;
    }
    if (!found->available()) {
        m_notification->showMessage(tr("Открытие работ Мили → Мура пока недоступно."), UiFeedbackLevel::Warning);
        return false;
    }
    if (!loadArchive(archivePath) || !setupWorkspace(*found)) return false;
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
    m_teacherId->setText(report.value(QStringLiteral("teacherId")).toString());
    m_studentName->setText(report.value(QStringLiteral("studentName")).toString());
    m_studentId->setText(report.value(QStringLiteral("studentId")).toString());
    m_group->setText(report.value(QStringLiteral("group")).toString());
    m_recordBook->setText(report.value(QStringLiteral("recordBook")).toString());
    m_progressPath = progressPath;
    updateWorkflowState();
    return true;
}

void MainWindow::createReport() {
    if (!m_targetEditor->completed()) return;
    const QList<QLineEdit*> required = {m_teacherName, m_teacherId, m_studentName,
                                        m_studentId, m_group, m_recordBook};
    for (QLineEdit* edit : required) {
        if (edit->text().trimmed().isEmpty()) {
            setFeedback(m_reportFeedback, tr("Заполните все поля отчёта."), false);
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
    QByteArray pngData;
    QBuffer imageBuffer(&pngData);
    imageBuffer.open(QIODevice::WriteOnly);
    QImage::fromData(m_currentVariant.diagram).save(&imageBuffer, "PNG");
    const QString image = QString::fromLatin1(pngData.toBase64());
    const QString mime = QStringLiteral("image/png");
    const QString html = QStringLiteral(
        "<!doctype html><html lang='ru'><meta charset='utf-8'><title>Отчёт M2MConvertor</title>"
        "<style>body{font:16px sans-serif;max-width:900px;margin:40px auto;color:#172033}"
        "table{border-collapse:collapse}td,th{border:1px solid #aaa;padding:7px}img{max-width:100%}</style>"
        "<h1>Преобразование эквивалентных автоматов</h1>"
        "<p><b>Вариант:</b> №%1<br><b>Направление:</b> %2<br><b>Дата:</b> %3</p>"
        "<h2>Участники</h2><table><tr><th>Роль</th><th>Ф. И. О.</th><th>Идентификатор</th></tr>"
        "<tr><td>Преподаватель</td><td>%4</td><td>%5</td></tr>"
        "<tr><td>Студент</td><td>%6</td><td>%7</td></tr></table>"
        "<p><b>Группа:</b> %8<br><b>Зачётная книжка:</b> %9</p>"
        "<h2>Результат</h2><p>%10</p><p>%11</p>"
        "<p><b>Проверка:</b> таблица, матрица и граф обоих автоматов приняты; "
        "реакция каждого введённого автомата из a0 совпадает с реакцией варианта. "
        "Проверена заданная последовательность входов; эквивалентность на всех "
        "возможных последовательностях этой проверкой не устанавливается.</p>"
        "<h2>Задание</h2><img src='data:%12;base64,%13'></html>")
        .arg(m_currentVariant.number)
        .arg(conversionDirectionName(m_currentVariant.direction).toHtmlEscaped())
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate).toHtmlEscaped())
        .arg(m_teacherName->text().toHtmlEscaped(), m_teacherId->text().toHtmlEscaped(),
             m_studentName->text().toHtmlEscaped(), m_studentId->text().toHtmlEscaped(),
             m_group->text().toHtmlEscaped(), m_recordBook->text().toHtmlEscaped(),
             automatonSummary(m_sourceEditor->acceptedAutomaton()).toHtmlEscaped(),
             automatonSummary(m_targetEditor->acceptedAutomaton()).toHtmlEscaped(), mime, image);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(html.toUtf8()) != html.toUtf8().size() || !file.commit()) {
        setFeedback(m_reportFeedback, tr("Не удалось сохранить отчёт: %1").arg(file.errorString()), false);
        return;
    }
    setFeedback(m_reportFeedback, tr("Отчёт создан: %1").arg(path), true);
}

void MainWindow::setFeedback(FeedbackLabel* label, const QString& text, bool success) {
    label->showMessage(text, success ? UiFeedbackLevel::Success : UiFeedbackLevel::Error);
    if (success) m_notification->showMessage(text, UiFeedbackLevel::Success);
}
