#include "mainwindow.h"

#include "domain/conversionservice.h"
#include "infrastructure/progressstore.h"
#include "ui/automatoneditor.h"

#include <QAction>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
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
#include <QStackedWidget>
#include <QStatusBar>
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
    resize(1100, 780);
    setMinimumSize(850, 620);
    m_pages = new QStackedWidget;
    m_pages->addWidget(createLandingPage());
    m_pages->addWidget(createWorkspacePage());
    setCentralWidget(m_pages);
    createMenus();
    setStyleSheet(QStringLiteral(
        "QMainWindow{background:#f8fafc}"
        "QGroupBox{font-weight:600;border:1px solid #cbd5e1;border-radius:7px;"
        "margin-top:10px;padding-top:9px;background:white}"
        "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px}"
        "QPushButton{padding:7px 14px;border:1px solid #94a3b8;border-radius:5px;background:#fff}"
        "QPushButton:hover{background:#eff6ff;border-color:#3b82f6}"
        "QPushButton:disabled{color:#94a3b8;background:#f1f5f9}"
        "QLineEdit,QComboBox,QSpinBox{padding:5px;border:1px solid #94a3b8;border-radius:4px}"
        "QTabWidget::pane{border:1px solid #cbd5e1;background:white}"
        "QTabBar::tab{padding:8px 14px}"
        "QTabBar::tab:selected{background:white;color:#1d4ed8;font-weight:600}"));
}

QWidget* MainWindow::createLandingPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->addStretch();
    auto* panel = new QGroupBox(tr("Новая учебная работа"));
    panel->setMaximumWidth(720);
    auto* layout = new QVBoxLayout(panel);
    auto* title = new QLabel(tr("Преобразование конечных автоматов Мили и Мура"));
    QFont titleFont = title->font();
    titleFont.setPointSize(17);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    auto* explanation = new QLabel(tr(
        "Откройте зашифрованную базу BazaVar, созданную в M2MTools, и выберите вариант. "
        "Направление преобразования определяется самим вариантом."));
    explanation->setWordWrap(true);
    explanation->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    layout->addWidget(explanation);
    auto* archiveRow = new QHBoxLayout;
    m_archivePathLabel = new QLabel(tr("BazaVar не выбран"));
    m_archivePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* archiveButton = new QPushButton(tr("Открыть BazaVar…"));
    archiveButton->setObjectName(QStringLiteral("openArchiveButton"));
    archiveRow->addWidget(m_archivePathLabel, 1);
    archiveRow->addWidget(archiveButton);
    layout->addLayout(archiveRow);
    auto* form = new QFormLayout;
    m_variantCombo = new QComboBox;
    m_variantCombo->setEnabled(false);
    m_landingDirection = new QLabel(tr("—"));
    form->addRow(tr("Вариант:"), m_variantCombo);
    form->addRow(tr("Преобразование:"), m_landingDirection);
    layout->addLayout(form);
    auto* actions = new QHBoxLayout;
    m_beginButton = new QPushButton(tr("Создать работу"));
    m_beginButton->setObjectName(QStringLiteral("beginWorkButton"));
    m_beginButton->setEnabled(false);
    auto* openProgressButton = new QPushButton(tr("Открыть сохранённую работу…"));
    openProgressButton->setObjectName(QStringLiteral("openProgressButton"));
    actions->addStretch();
    actions->addWidget(m_beginButton);
    actions->addWidget(openProgressButton);
    actions->addStretch();
    layout->addLayout(actions);
    auto* centered = new QHBoxLayout;
    centered->addStretch();
    centered->addWidget(panel);
    centered->addStretch();
    outer->addLayout(centered);
    outer->addStretch();
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
    auto* back = new QPushButton(tr("← К выбору варианта"));
    m_variantTitle = new QLabel;
    QFont font = m_variantTitle->font();
    font.setPointSize(13);
    font.setBold(true);
    m_variantTitle->setFont(font);
    top->addWidget(back);
    top->addWidget(m_variantTitle);
    top->addStretch();
    layout->addLayout(top);
    m_workflowTabs = new QTabWidget;
    m_workflowTabs->setObjectName(QStringLiteral("workflowTabs"));
    m_workflowTabs->addTab(createStartPage(), tr("Начало работы"));
    m_sourceEditor = new AutomatonEditor;
    m_targetEditor = new AutomatonEditor;
    m_workflowTabs->addTab(m_sourceEditor, tr("Исходный автомат"));
    m_workflowTabs->addTab(m_targetEditor, tr("Эквивалентный автомат"));
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
    auto* layout = new QHBoxLayout(page);
    auto* left = new QVBoxLayout;
    m_directionLabel = new QLabel;
    QFont directionFont = m_directionLabel->font();
    directionFont.setPointSize(15);
    directionFont.setBold(true);
    m_directionLabel->setFont(directionFont);
    m_directionLabel->setAlignment(Qt::AlignCenter);
    m_diagramLabel = new QLabel(tr("В варианте отсутствует изображение"));
    m_diagramLabel->setAlignment(Qt::AlignCenter);
    m_diagramLabel->setMinimumSize(420, 320);
    m_diagramLabel->setStyleSheet(QStringLiteral("border:1px solid #cbd5e1;background:white"));
    left->addWidget(m_directionLabel);
    left->addWidget(m_diagramLabel, 1);
    auto* counts = new QGroupBox(tr("Проверьте структуру исходного автомата"));
    auto* countLayout = new QFormLayout(counts);
    m_stateCount = new QSpinBox;
    m_inputCount = new QSpinBox;
    m_outputCount = new QSpinBox;
    for (QSpinBox* spin : {m_stateCount, m_inputCount, m_outputCount})
        spin->setRange(1, 200);
    countLayout->addRow(tr("Количество состояний:"), m_stateCount);
    countLayout->addRow(tr("Количество входных сигналов:"), m_inputCount);
    countLayout->addRow(tr("Количество выходных сигналов:"), m_outputCount);
    auto* accept = new QPushButton(tr("Принять"));
    accept->setObjectName(QStringLiteral("acceptCountsButton"));
    m_startFeedback = new QLabel;
    m_startFeedback->setWordWrap(true);
    countLayout->addRow(accept);
    countLayout->addRow(m_startFeedback);
    auto* right = new QVBoxLayout;
    auto* instructions = new QLabel(tr(
        "Изучите задание на рисунке и укажите размер исходного автомата. После правильного "
        "ответа откроется последовательная работа с таблицей, матрицей и графом."));
    instructions->setWordWrap(true);
    right->addWidget(instructions);
    right->addWidget(counts);
    right->addStretch();
    layout->addLayout(left, 3);
    layout->addLayout(right, 2);
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
    m_reportFeedback = new QLabel;
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
    auto* file = menuBar()->addMenu(tr("Файл"));
    auto* newWork = file->addAction(tr("Выбрать другой вариант"));
    auto* open = file->addAction(tr("Открыть работу…"));
    m_saveAction = file->addAction(tr("Сохранить работу"));
    m_saveAsAction = file->addAction(tr("Сохранить работу как…"));
    file->addSeparator();
    auto* quit = file->addAction(tr("Выход"));
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
        if (showErrors) QMessageBox::critical(this, tr("Не удалось открыть BazaVar"), error);
        return false;
    }
    if (variants.isEmpty()) {
        if (showErrors) QMessageBox::warning(this, tr("Пустая база"),
                                             tr("В BazaVar нет вариантов."));
        return false;
    }
    m_archivePath = QFileInfo(path).absoluteFilePath();
    m_variants = variants;
    m_archivePathLabel->setText(QFileInfo(path).fileName());
    m_archivePathLabel->setToolTip(m_archivePath);
    m_variantCombo->blockSignals(true);
    m_variantCombo->clear();
    for (const TrainingVariant& variant : variants)
        m_variantCombo->addItem(tr("Вариант №%1").arg(variant.number), variant.number);
    m_variantCombo->setEnabled(true);
    m_variantCombo->blockSignals(false);
    updateVariantSelection(0);
    return true;
}

void MainWindow::updateVariantSelection(int index) {
    const bool valid = index >= 0 && index < m_variants.size();
    m_beginButton->setEnabled(valid);
    m_landingDirection->setText(valid
        ? conversionDirectionName(m_variants[index].direction) : QStringLiteral("—"));
}

void MainWindow::beginWork() {
    const int index = m_variantCombo->currentIndex();
    if (index < 0 || index >= m_variants.size()) return;
    setupWorkspace(m_variants[index]);
    m_progressPath.clear();
    m_pages->setCurrentIndex(kWorkspacePage);
}

void MainWindow::setupWorkspace(const TrainingVariant& variant) {
    m_currentVariant = variant;
    m_hasVariant = true;
    m_countsAccepted = false;
    QString error;
    if (!ConversionService::convert(variant.source, &m_expectedTarget, &error)) {
        QMessageBox::critical(this, tr("Ошибка варианта"), error);
        m_hasVariant = false;
        return;
    }
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
    m_startFeedback->clear();
    m_sourceEditor->setExpected(variant.source);
    m_targetEditor->setExpected(m_expectedTarget);
    m_workflowTabs->setCurrentIndex(0);
    m_teacherName->clear(); m_teacherId->clear();
    m_studentName->clear(); m_studentId->clear();
    m_group->clear(); m_recordBook->clear();
    m_reportFeedback->clear();
    m_saveAction->setEnabled(true);
    m_saveAsAction->setEnabled(true);
    updateWorkflowState();
}

void MainWindow::acceptCounts() {
    if (!m_hasVariant) return;
    const Automaton& source = m_currentVariant.source;
    const bool correct = m_stateCount->value() == source.states.size() &&
                         m_inputCount->value() == source.inputs.size() &&
                         m_outputCount->value() == source.outputs.size();
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
        QMessageBox::critical(this, tr("Ошибка сохранения"), error);
        return;
    }
    m_progressPath = path;
    statusBar()->showMessage(tr("Работа сохранена: %1").arg(QFileInfo(path).fileName()), 5000);
}

void MainWindow::openProgress() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Открыть учебную работу"), QString(),
        tr("Работа M2MConvertor (*.m2mwork);;Все файлы (*)"));
    if (path.isEmpty()) return;
    QJsonObject data;
    QString error;
    if (!ProgressStore::load(path, &data, &error)) {
        QMessageBox::critical(this, tr("Ошибка открытия"), error);
        return;
    }
    if (!restoreProgress(data, path)) return;
    m_pages->setCurrentIndex(kWorkspacePage);
}

bool MainWindow::restoreProgress(const QJsonObject& data, const QString& progressPath) {
    QString archivePath = data.value(QStringLiteral("archivePath")).toString();
    if (!loadArchive(archivePath, false)) {
        archivePath = QFileDialog::getOpenFileName(
            this, tr("Укажите BazaVar для сохранённой работы"), QString(),
            tr("BazaVar M2MTools (*.m2m);;Все файлы (*)"));
        if (archivePath.isEmpty() || !loadArchive(archivePath)) return false;
    }
    const int variantNumber = data.value(QStringLiteral("variantNumber")).toInt();
    auto found = std::find_if(m_variants.cbegin(), m_variants.cend(),
        [variantNumber](const TrainingVariant& variant) { return variant.number == variantNumber; });
    if (found == m_variants.cend()) {
        QMessageBox::critical(this, tr("Вариант не найден"),
                              tr("В текущем BazaVar нет варианта №%1.").arg(variantNumber));
        return false;
    }
    setupWorkspace(*found);
    if (!m_hasVariant) return false;
    m_stateCount->setValue(data.value(QStringLiteral("stateCount")).toInt(1));
    m_inputCount->setValue(data.value(QStringLiteral("inputCount")).toInt(1));
    m_outputCount->setValue(data.value(QStringLiteral("outputCount")).toInt(1));
    m_countsAccepted = data.value(QStringLiteral("countsAccepted")).toBool() &&
        m_stateCount->value() == m_currentVariant.source.states.size() &&
        m_inputCount->value() == m_currentVariant.source.inputs.size() &&
        m_outputCount->value() == m_currentVariant.source.outputs.size();
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
    const QString image = QString::fromLatin1(m_currentVariant.diagram.toBase64());
    const QString mime = m_currentVariant.diagramFileName.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)
        ? QStringLiteral("image/png") : QStringLiteral("image/jpeg");
    const QString html = QStringLiteral(
        "<!doctype html><html lang='ru'><meta charset='utf-8'><title>Отчёт M2MConvertor</title>"
        "<style>body{font:16px sans-serif;max-width:900px;margin:40px auto;color:#172033}"
        "table{border-collapse:collapse}td,th{border:1px solid #aaa;padding:7px}img{max-width:100%%}</style>"
        "<h1>Преобразование эквивалентных автоматов</h1>"
        "<p><b>Вариант:</b> №%1<br><b>Направление:</b> %2<br><b>Дата:</b> %3</p>"
        "<h2>Участники</h2><table><tr><th>Роль</th><th>Ф. И. О.</th><th>Идентификатор</th></tr>"
        "<tr><td>Преподаватель</td><td>%4</td><td>%5</td></tr>"
        "<tr><td>Студент</td><td>%6</td><td>%7</td></tr></table>"
        "<p><b>Группа:</b> %8<br><b>Зачётная книжка:</b> %9</p>"
        "<h2>Результат</h2><p>%10</p><p>%11</p>"
        "<p><b>Проверка:</b> таблица, матрица и граф обоих автоматов приняты; "
        "эквивалентный автомат изоморфен автоматически построенному эталону.</p>"
        "<h2>Задание</h2><img src='data:%12;base64,%13'></html>")
        .arg(m_currentVariant.number)
        .arg(conversionDirectionName(m_currentVariant.direction).toHtmlEscaped())
        .arg(QDateTime::currentDateTime().toString(Qt::ISODate).toHtmlEscaped())
        .arg(m_teacherName->text().toHtmlEscaped(), m_teacherId->text().toHtmlEscaped(),
             m_studentName->text().toHtmlEscaped(), m_studentId->text().toHtmlEscaped(),
             m_group->text().toHtmlEscaped(), m_recordBook->text().toHtmlEscaped(),
             automatonSummary(m_currentVariant.source).toHtmlEscaped(),
             automatonSummary(m_expectedTarget).toHtmlEscaped(), mime, image);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(html.toUtf8()) != html.toUtf8().size() || !file.commit()) {
        setFeedback(m_reportFeedback, tr("Не удалось сохранить отчёт: %1").arg(file.errorString()), false);
        return;
    }
    setFeedback(m_reportFeedback, tr("Отчёт создан: %1").arg(path), true);
}

void MainWindow::setFeedback(QLabel* label, const QString& text, bool success) {
    label->setText(text);
    label->setStyleSheet(success ? QStringLiteral("color:#15803d;font-weight:600")
                                 : QStringLiteral("color:#b91c1c;font-weight:600"));
}
