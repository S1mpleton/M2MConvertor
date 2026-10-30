#include "infrastructure/translationmanager.h"
#include "infrastructure/logger.h"
#include "infrastructure/progressstore.h"
#include "ui/logwindow.h"
#include <QAction>
#include <QMenu>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QClipboard>
#include <QSettings>
#include "mainwindow.h"
#include "ui/automatongrapheditor.h"
#include "ui/automatoneditor.h"
#include "ui/appicons.h"
#include "ui/contraststyle.h"
#include "ui/mealycelldelegate.h"
#include "ui/widgets/taskimage.h"
#include "ui/widgets/hintsplitter.h"
#include "ui/widgets/workscrollarea.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QBuffer>
#include <QToolButton>
#include "ui/widgets/feedbacklabel.h"
#include <QGraphicsOpacityEffect>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QHelpEvent>
#include <QToolTip>
#include <QMenuBar>
#include <QVBoxLayout>

#include <QApplication>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QFile>
#include <QTimer>
#include <QStyle>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonArray>
#include <QLineF>
#include <QPalette>
#include <QPushButton>
#include <QScreen>
#include <QFormLayout>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTest>

#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

#include "tests/archivefixture.h"

struct MainWindowTestAccess {
    static bool open(MainWindow& window, const QString& path) {
        return window.loadArchive(path, false);
    }
    static bool setup(MainWindow& window, const TrainingVariant& variant) {
        return window.setupWorkspace(variant);
    }
    static void begin(MainWindow& window) { window.beginWork(); }
    static void acceptCounts(MainWindow& window) {
        window.m_stateCount->setValue(window.m_currentVariant.moore.states);
        window.m_inputCount->setValue(window.m_currentVariant.moore.inputs);
        window.m_outputCount->setValue(window.m_currentVariant.moore.outputs);
        window.acceptCounts();
    }
    static QJsonObject progress(MainWindow& window) { return window.progressData(); }
    static bool dirty(MainWindow& window) { return window.hasUnsavedChanges(); }
    static bool save(MainWindow& window, const QString& path) {
        window.m_progressPath = path;
        return window.saveProgress();
    }
    static void back(MainWindow& window) { window.returnToLanding(); }
    static bool restore(MainWindow& window, const QJsonObject& data) {
        return window.restoreProgress(data, QString());
    }
    static QString html(MainWindow& window) {
        window.m_teacherName->setText("Преподаватель <&>");
        window.m_studentName->setText("Студент"); window.m_group->setText("Группа");
        window.m_recordBook->setText("1234");
        return window.reportHtml();
    }
};

QString hoverHelp(QTableWidget* table, int row, int col) {
    QApplication::processEvents();
    const auto point = table->visualItemRect(table->item(row, col)).center();
    QHelpEvent event(QEvent::ToolTip, point, table->viewport()->mapToGlobal(point));
    QApplication::sendEvent(table->viewport(), &event);
    const auto text = QToolTip::text();
    QToolTip::hideText();
    return text;
}

void requireVisibleAction(QWidget* window, QPushButton* button) {
    require(button && button->isVisible(), "working button remains visible");
    if (!button->visibleRegion().boundingRect().contains(button->rect()))
        qWarning() << "clipped action" << button->objectName() << window->size() << button->geometry()
                   << button->visibleRegion().boundingRect() << button->parentWidget()->size();
    require(button->visibleRegion().boundingRect().contains(button->rect()), "working button is not clipped by a viewport");
    require(button->width() >= button->minimumSizeHint().width() && button->height() >= button->sizeHint().height(),
        "working button retains its full caption and height");
    for (auto* parent = button->parentWidget(); parent; parent = parent->parentWidget()) {
        require(parent->rect().contains(QRect(button->mapTo(parent, QPoint()), button->size())),
            "working button fits every containing widget");
        if (parent == window) break;
    }
}

void testReactionWorkflow(MainWindow& window, const QString& screenshot) {
    window.activateWindow(); window.raise(); QApplication::processEvents();
    QTemporaryDir temporary;
    auto variant = trainingVariantJson();
    const QJsonObject counts{{"states", 1}, {"inputs", 1}, {"outputs", 1}};
    variant.insert("mealy", counts); variant.insert("moore", counts);
    variant.insert("reaction", QJsonArray{QJsonObject{{"input", "x9"}, {"outputs", QJsonArray{"y4"}}}});
    auto blocked = variant; blocked.insert("number", 1); blocked.insert("type", "MealyToMoore");
    auto root = trainingArchiveJson(); root.insert("variants", QJsonArray{blocked, variant});
    const auto archivePath = temporary.filePath("tasks.m2m");
    QFile file(archivePath); require(file.open(QIODevice::WriteOnly), "archive opens");
    file.write(makeCompatibleArchive(root)); file.close();
    require(MainWindowTestAccess::open(window, archivePath), "new archive opens");
    auto* combo = window.findChild<QComboBox*>("variantCombo");
    require(combo->currentData().toInt() == 7 &&
        !(combo->model()->flags(combo->model()->index(0, 0)) & Qt::ItemIsEnabled), "unfinished direction disabled");
    MainWindowTestAccess::begin(window);
    QApplication::processEvents();
    QApplication::processEvents();
    QApplication::processEvents();
    auto* countForm = window.findChild<QFormLayout*>("sourceCountsForm");
    for (int row = 0; row < 3; ++row) {
        auto* label = countForm->itemAt(row, QFormLayout::LabelRole)->widget();
        auto* field = countForm->itemAt(row, QFormLayout::FieldRole)->widget();
        require(qAbs(label->geometry().center().y() - field->geometry().center().y()) <= 2,
            "count fields stay on the same row as their labels at the startup size");
    }
    require(window.findChild<WorkScrollArea*>("startWorkArea")->horizontalScrollBar()->maximum() == 0,
        "startup splitter leaves enough width to show the whole count form");
    require(window.screen()->availableGeometry().contains(window.frameGeometry()), "opening a task keeps the frame on screen");
    if (window.frameGeometry().height() < window.screen()->availableGeometry().height() - 1)
        require(window.findChild<WorkScrollArea*>("startWorkArea")->verticalScrollBar()->maximum() == 0,
            "startup form fits vertically when the screen has enough room");
    if (!screenshot.isEmpty()) window.grab().save(screenshot + ".compact-start.png");
    window.resize(1100, 820); QApplication::processEvents();
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".start.png");
    }
    MainWindowTestAccess::acceptCounts(window);
    auto* source = window.findChild<AutomatonEditor*>("sourceEditor");
    auto* target = window.findChild<AutomatonEditor*>("targetEditor");
    auto* workflow = window.findChild<QTabWidget*>("workflowTabs");
    auto* table = source->findChild<QTableWidget*>("transitionTable");
    require(!source->findChild<QLineEdit*>("inputSignals") && table->rowCount() == 5 &&
        table->columnCount() == 2 && table->item(3, 1)->text() == "a0" &&
        !(table->item(3, 1)->flags() & Qt::ItemIsEditable) &&
        table->item(4, 0)->text().isEmpty() && table->item(4, 1)->text() == "-",
        "table auto-generates canonical header, blank input and absent transitions");
    const auto answersBeforeHelp = source->answerData();
    require(hoverHelp(table, 4, 0).contains("Один входной сигнал") &&
        hoverHelp(table, 1, 1).contains("y0, y2") &&
        hoverHelp(table, 4, 1).contains("Одно следующее состояние"),
        "Moore cells show examples for inputs, outputs and destination states");
    require(source->answerData() == answersBeforeHelp, "hovering never changes answers");
    table->editItem(table->item(4, 0)); QApplication::processEvents();
    auto* freeInput = table->findChild<QLineEdit*>();
    require(freeInput && !freeInput->validator(), "Moore input has no typing validator");
    QTest::keyClicks(freeInput, "bad @ input"); QTest::keyClick(freeInput, Qt::Key_Return); QApplication::processEvents();
    require(table->item(4, 0)->text() == "bad @ input", "invalid drafts are saved unchanged");
    table->editItem(table->item(4, 0)); QApplication::processEvents();
    for (auto* edit : table->findChildren<QLineEdit*>()) {
        if (!edit->isVisible()) continue;
        edit->clear(); QTest::keyClick(edit, Qt::Key_Return); break;
    }
    QApplication::processEvents();
    require(table->item(4, 0)->text() == "-", "cleared single field becomes dash on commit");
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(!source->toJson().value("tableAccepted").toBool() &&
        window.findChild<FeedbackLabel*>("applicationNotification")->text().contains("x0"),
        "invalid input is reported on Accept in the shared footer");
    table->item(1, 1)->setText("y4"); table->item(4, 0)->setText("x9");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".table.png");
    }
    table->item(4, 1)->setText(" ");
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(table->item(4, 1)->text() == "-", "empty Moore cell normalizes on Accept");
    require(source->toJson().value("tableAccepted").toBool(), "screenshot's table without a transition is accepted");
    table->item(4, 1)->setText("a0");
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(source->toJson().value("tableAccepted").toBool(), "table with final loop is also accepted");
    auto* matrix = source->findChild<QTableWidget*>("transitionMatrix");
    auto* outputs = source->findChild<QTableWidget*>("matrixOutputs");
    auto* dimension = source->findChild<QLineEdit*>("matrixDimensionEdit");
    auto* build = source->findChild<QPushButton*>("configureMatrixButton");
    auto* clear = source->findChild<QPushButton*>("clearMatrixButton");
    require(matrix->rowCount() == 0 && outputs->rowCount() == 0 && dimension->text().isEmpty() && !clear->isEnabled(),
        "matrix waits for the user's dimension instead of building from the table");
    for (const auto& invalid : {"text", "1.5", "0", "2"}) {
        dimension->setText(invalid); build->click();
        require(matrix->rowCount() == 0, "invalid or mismatched dimensions do not create a matrix");
    }
    {
        MainWindow draft;
        require(MainWindowTestAccess::restore(draft, MainWindowTestAccess::progress(window)), "work waiting for a matrix dimension restores");
        auto* restoredEditor = draft.findChild<AutomatonEditor*>("sourceEditor");
        require(restoredEditor->findChild<QTableWidget*>("transitionMatrix")->rowCount() == 0 &&
            restoredEditor->findChild<QLineEdit*>("matrixDimensionEdit")->text() == "2", "restoring does not auto-build an unfinished matrix");
    }
    dimension->setText("1"); build->click();
    require(hoverHelp(matrix, 0, 0).contains("x0, x2") && hoverHelp(outputs, 0, 1).contains("y0, y2"),
        "Moore matrices show separate edge-input and output examples");
    require(hoverHelp(source->findChild<QTableWidget*>("matrixReference"), 4, 1).contains("только для просмотра"),
        "accepted reference table explains that its cells are read-only");
    require(matrix->item(0, 0)->text() == "-", "matrix defaults to no transitions");
    outputs->item(0, 1)->setText("y4");
    build->click();
    require(outputs->item(0, 1)->text() == "y4", "accepting the same dimension preserves entered cells");
    matrix->item(0, 0)->setText("x9"); clear->click();
    require(matrix->rowCount() == 1 && matrix->item(0, 0)->text() == "-" && outputs->item(0, 1)->text() == "-",
        "Clear resets both matrices while keeping their dimension");
    outputs->item(0, 1)->setText("y4");
    bool c = false, y = false;
    for (auto* label : source->findChildren<QLabel*>()) { c |= label->text() == "C ="; y |= label->text() == "Y ="; }
    require(c && y, "matrix and output table have mathematical captions");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".matrix.png");
        const auto dark = QApplication::palette();
        QApplication::setPalette(QApplication::style()->standardPalette());
        QApplication::processEvents(); window.grab().save(screenshot + ".matrix-light.png");
        QApplication::setPalette(dark);
    }
    source->findChild<QPushButton*>("acceptMatrixButton")->click();
    require(source->toJson().value("matrixAccepted").toBool(), "matrix is checked independently without inheriting final loop");
    auto* graph = source->findChild<AutomatonGraphEditor*>();
    graph->createVertex(); graph->createVertex();
    require(graph->vertices().size() == 2, "graph can exceed task's state count");
    auto data = graph->toJson(); auto vertices = data.value("vertices").toArray();
    auto first = vertices[0].toObject(); first.insert("label", "a0 / y4"); vertices[0] = first;
    data.insert("vertices", vertices); QString error;
    require(graph->fromJson(data, &error), "graph draft restores");
    source->findChild<QPushButton*>("acceptGraphButton")->click();
    require(source->completed() && workflow->isTabEnabled(2), "graph with extra unobserved state is accepted by reaction");
    auto* graphReference = source->findChild<QTableWidget*>("graphReference");
    require(graphReference && graphReference->item(4, 0)->text() == "x9" &&
        graphReference->item(4, 1)->text() == "a0" &&
        graphReference->editTriggers() == QAbstractItemView::NoEditTriggers,
        "graph shows the accepted table as read-only reference");
    auto* instructions = source->findChild<QLabel*>("graphInstructions");
    QApplication::processEvents();
    require(instructions->geometry().top() > graph->geometry().bottom(), "graph instructions do not overlap the canvas");
    auto* divider = source->findChild<HintSplitter*>("graphHint");

    require(divider && !source->findChild<QToolButton*>("graphHintToggle"), "hint uses a splitter without a toggle button");
    auto* workArea = divider->widget(1)->findChild<WorkScrollArea*>();
    require(workArea && workArea->verticalScrollBar()->maximum() == 0 &&
        workArea->horizontalScrollBar()->maximum() == 0,
        "instructions and acceptance remain visible without outer scrolling in a regular window");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".graph-reference.png");
    }
    divider->setExpanded(false); QApplication::processEvents();
    require(divider->hintExtent() == 0 && source->completed(), "hiding graph reference keeps acceptance");
    auto* t = target->findChild<QTableWidget*>("transitionTable");
    require(t->item(2, 1)->text() == "-/-", "Mealy transitions default to -/-");
    workflow->setCurrentIndex(2); QApplication::processEvents();
    require(hoverHelp(t, 2, 1).contains("a0 / y0, y2") && hoverHelp(t, 2, 1).contains("-/-"),
        "Mealy table explains both sides and the absent transition format");
    t->item(2, 0)->setText("x9");
    t->editItem(t->item(2, 1)); QApplication::processEvents();
    auto activeEditor = [t]() -> QWidget* {
        for (auto* candidate : t->findChildren<QWidget*>("mealyCellEditor"))
            if (candidate->isVisible()) return candidate;
        return nullptr;
    };
    auto* editor = activeEditor();
    require(editor, "Mealy cell opens two-field editor");
    auto* stateEdit = editor->findChild<QLineEdit*>("destinationEditor");
    auto* outputEdit = editor->findChild<QLineEdit*>("outputsEditor");
    require(stateEdit && outputEdit && !stateEdit->validator() && !outputEdit->validator(), "neither Mealy field has a validator");
    stateEdit->setText(""); outputEdit->setText(" ");
    QTest::keyClick(outputEdit, Qt::Key_Return); QApplication::processEvents();
    require(t->item(2, 1)->text() == "-/-", "empty Mealy fields become dash/dash");
    t->editItem(t->item(2, 1)); QApplication::processEvents();
    editor = activeEditor(); require(editor, "Mealy editor reopens");
    stateEdit = editor->findChild<QLineEdit*>("destinationEditor");
    outputEdit = editor->findChild<QLineEdit*>("outputsEditor");
    stateEdit->setText("a0, a1"); outputEdit->setText("y4");
    QTest::keyClick(outputEdit, Qt::Key_Return); QApplication::processEvents();
    require(t->item(2, 1)->text() == "a0, a1/y4", "both raw Mealy fields commit without filtering");
    target->findChild<QPushButton*>("acceptTableButton")->click();
    require(!target->toJson().value("tableAccepted").toBool(), "multiple destination states rejected on Accept");
    t->item(2, 1)->setText("-/y4");
    target->findChild<QPushButton*>("acceptTableButton")->click();
    require(!target->toJson().value("tableAccepted").toBool(), "outputs without a destination are rejected");
    t->item(2, 1)->setText("a0/y4,@");
    target->findChild<QPushButton*>("acceptTableButton")->click();
    require(!target->toJson().value("tableAccepted").toBool(), "invalid output symbols are rejected on Accept");
    t->editItem(t->item(2, 1)); QApplication::processEvents();
    editor = activeEditor();
    require(editor, "a new composite editor opens");
    stateEdit = editor->findChild<QLineEdit*>("destinationEditor");
    outputEdit = editor->findChild<QLineEdit*>("outputsEditor");
    stateEdit->setText("a0"); outputEdit->setText("y4");
    outputEdit->setFocus();
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".mealy-editor.png");
    }
    // Clicking Accept must commit the active composite editor before checking it.

    target->findChild<QPushButton*>("acceptTableButton")->click();
    require(target->toJson().value("tableAccepted").toBool(), "Mealy table accepted");
    require(target->findChild<QTableWidget*>("transitionMatrix")->rowCount() == 0, "Mealy matrix also waits for a dimension");
    target->findChild<QLineEdit*>("matrixDimensionEdit")->setText("1");
    target->findChild<QPushButton*>("configureMatrixButton")->click();
    require(hoverHelp(target->findChild<QTableWidget*>("transitionMatrix"), 0, 0).contains("x0 / y0, y2; x1 / -"),
        "Mealy matrix shows semicolon-separated edge examples");
    target->findChild<QTableWidget*>("transitionMatrix")->item(0, 0)->setText("x9/y4");
    target->findChild<QPushButton*>("acceptMatrixButton")->click();
    auto* g = target->findChild<AutomatonGraphEditor*>(); g->createVertex();
    auto gd = g->toJson(); gd.insert("edges", QJsonArray{QJsonObject{{"from", 1}, {"to", 1}, {"label", "x9/y4"}}});
    require(g->fromJson(gd, &error), "Mealy graph restores");
    target->findChild<QPushButton*>("acceptGraphButton")->click();
    require(target->completed() && workflow->isTabEnabled(3), "six accepted representations unlock report");
    const auto acceptedSource = source->acceptedAutomaton().toJson();
    const auto acceptedTarget = target->acceptedAutomaton().toJson();
    const auto hintState = source->toJson().value("hintExtents");
    auto* settingsMenu = window.findChild<QMenu*>("settingsMenu");
    auto* english = window.findChild<QAction*>("languageEnglishAction");
    auto* russian = window.findChild<QAction*>("languageRussianAction");
    require(settingsMenu && english && russian, "Settings exposes language actions");
    require(MainWindowTestAccess::save(window, temporary.filePath("accepted-work.m2mwork")), "completed work saves before language switch");
    window.findChild<QAction*>("eventLogAction")->trigger(); QApplication::processEvents();
    auto* log = window.findChild<LogWindow*>("eventLogWindow");
    require(log && log->windowTitle() == "Журнал событий", "Settings opens translated event log");
    english->trigger(); QApplication::processEvents();
    require(hoverHelp(target->findChild<QTableWidget*>("graphReference"), 2, 1).contains("read-only"),
        "cell hints follow a live language switch");
    require(!MainWindowTestAccess::dirty(window), "translated generated table headings do not create unsaved changes");
    if (!screenshot.isEmpty()) {
        window.grab().save(screenshot + ".english.png");
        log->grab().save(screenshot + ".log-english.png");
    }
    require(settingsMenu->title() == "Settings" && workflow->tabText(1) == "Moore automaton" &&
        source->findChild<QTabWidget*>()->tabText(0) == "Table" && table->item(0, 0)->text() == "Input signals",
        "live translation covers menus, workflow tabs and generated headers");
    require(source->completed() && target->completed() && acceptedSource == source->acceptedAutomaton().toJson() &&
        acceptedTarget == target->acceptedAutomaton().toJson() && hintState == source->toJson().value("hintExtents"),
        "language switch preserves answers, acceptance and divider sizes");
    require(table->item(4, 0)->text() == "x9" && table->item(3, 1)->text() == "a0", "user symbols and state names are never translated");
    auto* logText = log->findChild<QPlainTextEdit*>("logTextEdit");
    require(log->windowTitle() == "Application Log" && logText->toPlainText().contains("reaction from a0") &&
        logText->toPlainText().contains("Moore — Table") && !logText->toPlainText().contains("Верно."),
        "event history including formatted validation messages is retranslated");
    require(MainWindowTestAccess::html(window).contains("<html lang='en'>"), "HTML report follows selected language");
    require(QSettings().value("language").toString() == "en", "language preference is saved");
    require(!TranslationManager::instance().setLocale("fr", false).ok() && settingsMenu->title() == "Settings", "unsupported locale leaves active translation intact");
    russian->trigger(); QApplication::processEvents();
    require(settingsMenu->title() == "Настройки" && log->windowTitle() == "Журнал событий", "Russian can be restored live");
    for (auto* button : log->findChildren<QAbstractButton*>())
        require(button->width() >= button->minimumSizeHint().width(), "translated log controls retain enough space for their captions");
    if (!screenshot.isEmpty()) log->grab().save(screenshot + ".log-russian.png");
    log->hide(); window.activateWindow();
    require(workflow->widget(3)->findChildren<QLineEdit*>().size() == 4,
        "report has only names, group and record book");
    workflow->setCurrentIndex(3);
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".report.png");
    }
    const auto html = MainWindowTestAccess::html(window);
    require(!html.contains("Идентификатор") && !html.contains("%11") &&
        html.contains("Преподаватель &lt;&amp;&gt;") && html.contains("data:image/png;base64,"),
        "HTML report omits identifiers and keeps escaped names and embedded image");
    for (const auto& size : {QSize(1100, 820), window.minimumSizeHint(), QSize(640, 480), QSize(480, 360), QSize(900, 650)}) {
        window.resize(size); QApplication::processEvents();
        for (int editor = 0; editor < 2; ++editor) {
            workflow->setCurrentIndex(editor + 1);
            auto* representation = (editor == 0 ? source : target)->findChild<QTabWidget*>();
            for (int tab = 0; tab < 3; ++tab) {
                representation->setCurrentIndex(tab); QApplication::processEvents(); QApplication::processEvents();
                auto* active = representation->currentWidget();
                require(active->rect().contains(active->findChild<HintSplitter*>()->geometry()), "splitter stays inside page on shrink");
                const QStringList names = tab == 0 ? QStringList{"acceptTableButton"} : tab == 1 ?
                    QStringList{"clearMatrixButton", "configureMatrixButton", "acceptMatrixButton"} :
                    QStringList{"createVertexButton", "deleteVertexButton", "acceptGraphButton"};
                for (const auto& name : names) requireVisibleAction(&window, active->findChild<QPushButton*>(name));
                if (auto* scroll = active->findChild<WorkScrollArea*>()) {
                    scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
                    QApplication::processEvents();
                    for (const auto& name : names) requireVisibleAction(&window, active->findChild<QPushButton*>(name));
                }
                for (auto* label : active->findChildren<QLabel*>()) {
                    if (label->objectName() == "graphInstructions")
                        require(label->height() >= label->heightForWidth(label->width()) - 1, "wrapped instructions keep enough height");
                }
            }
        }
        workflow->setCurrentIndex(0); QApplication::processEvents();
        requireVisibleAction(&window, window.findChild<QPushButton*>("acceptCountsButton"));
        workflow->setCurrentIndex(3); QApplication::processEvents();
        requireVisibleAction(&window, window.findChild<QPushButton*>("createReportButton"));
    }
    workflow->setCurrentIndex(1); source->findChild<QTabWidget*>()->setCurrentIndex(2); QApplication::processEvents();
    if (!screenshot.isEmpty()) window.grab().save(screenshot + ".small-graph.png");
    MainWindow restored;
    require(MainWindowTestAccess::restore(restored, MainWindowTestAccess::progress(window)), "progress reloads");
    require(restored.findChild<AutomatonEditor*>("sourceEditor")->completed() &&
        restored.findChild<AutomatonEditor*>("targetEditor")->completed(), "new table and flexible graph revalidate after reload");
    require(!restored.findChild<AutomatonEditor*>("sourceEditor")->findChild<HintSplitter*>("graphHint")->expanded(),
        "hint visibility is preserved with the work");
    auto legacy = source->toJson(); legacy.remove("tableLayout");
    legacy.insert("tableInputs", QJsonArray{"x9"});
    legacy.insert("table", QJsonArray{QJsonArray{"y4"}, QJsonArray{"a0"}});
    auto* restoredSource = restored.findChild<AutomatonEditor*>("sourceEditor");
    restoredSource->fromJson(legacy);
    require(restoredSource->completed() &&
        restoredSource->findChild<QTableWidget*>("transitionTable")->item(4, 0)->text() == "x9",
        "version 2 drafts migrate and are rechecked");
    for (int i = 0; i < 20; ++i) graph->createVertex();
    require(graph->vertices().size() == 10, "graph is capped at ten states");
    source->findChild<QPushButton*>("deleteVertexButton")->click();
    require(graph->vertices().size() == 9 && graph->vertices().last().label.startsWith("a8"), "button deletes last vertex");
    graph->createVertex(); require(graph->vertices().last().label == "a9 / -", "last state name is reused");
    table->item(1, 1)->setText("y5");
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(!source->toJson().value("tableAccepted").toBool() && !workflow->isTabEnabled(2), "wrong output invalidates downstream work");
}

void testJournal() {
    auto& logger = Logger::instance(); logger.clear();
    const auto error = Result::error(Message(QT_TRANSLATE_NOOP("Result", "Такт %1: не задан вход %2.")).arg(4, QString("x9")), ResultCategory::Validation);
    logger.log(Result::success(Message(QT_TRANSLATE_NOOP("Result", "Приложение запущено."))));
    logger.log(Result::warning(Message(QT_TRANSLATE_NOOP("Result", "В BazaVar нет вариантов.")), ResultCategory::Archive));
    logger.log(error);
    LogWindow journal; journal.show(); QApplication::processEvents();
    auto* text = journal.findChild<QPlainTextEdit*>("logTextEdit");
    require(logger.history().size() == 3 && text->toPlainText().contains("Такт 4") && text->toPlainText().contains("x9"), "journal displays retained history with arguments");
    journal.findChild<QCheckBox*>("showInfoBox")->setChecked(false);
    require(!text->toPlainText().contains("Приложение запущено") && text->toPlainText().contains("Такт 4"), "severity filter hides only matching events");
    journal.findChild<QPushButton*>("copyButton")->click();
    require(QApplication::clipboard()->text() == text->toPlainText(), "copy without selection copies the filtered log");
    auto cursor = text->textCursor(); cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
    text->setTextCursor(cursor);
    const auto selected = cursor.selectedText();
    journal.findChild<QPushButton*>("copyButton")->click();
    require(QApplication::clipboard()->text() == selected, "copy respects selected log text");
    QTemporaryDir directory; const auto path = directory.filePath("журнал.txt");
    require(journal.saveTo(path).ok(), "log saves atomically as UTF-8");
    QFile file(path); require(file.open(QIODevice::ReadOnly) && QString::fromUtf8(file.readAll()) == text->toPlainText(), "saved log matches the visible filtered entries");
    require(!journal.saveTo(directory.filePath("missing/log.txt")).ok(), "failed log save returns an error");
    TranslationManager::instance().setLocale("en", false); QApplication::processEvents();
    require(error.text().contains("Step 4") && text->toPlainText().contains("Step 4") && text->toPlainText().contains("x9"), "stored Result translates its template before substituting arguments");
    journal.findChild<QPushButton*>("clearButton")->click();
    require(logger.history().isEmpty() && text->toPlainText().isEmpty(), "clear empties history and all views");
    TranslationManager::instance().setLocale("ru_RU", false);
}

void testBoundedGraph(const QString& screenshot) {
    AutomatonGraphEditor graph;
    graph.resize(700, 480); graph.show();
    for (int i = 0; i < 3; ++i) graph.createVertex();
    auto data = graph.toJson();
    data.insert("edges", QJsonArray{
        QJsonObject{{"from", 1}, {"to", 2}, {"label", "x0/y0"}},
        QJsonObject{{"from", 2}, {"to", 3}, {"label", "x1/-"}},
        QJsonObject{{"from", 2}, {"to", 2}, {"label", "x2/y0"}}});
    require(graph.fromJson(data), "stress graph loads with incoming, outgoing and loop edges");
    QApplication::processEvents();
    const auto canvas = graph.scene()->sceneRect();
    const auto initial = graph.vertices()[1].position;
    const auto start = graph.mapFromScene(initial);
    QTest::mousePress(graph.viewport(), Qt::LeftButton, Qt::NoModifier, start);
    for (int i = 0; i < 160; ++i) {
        const QPointF point((i & 1) ? -100000 : 100000, (i & 2) ? -100000 : 100000);
        QMouseEvent move(QEvent::MouseMove, point, graph.viewport()->mapToGlobal(point.toPoint()),
            Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(graph.viewport(), &move);
        if (i % 10 == 0) QApplication::processEvents();
        require(graph.scene()->sceneRect() == canvas, "dragging never changes scene or scroll bounds");
        for (const auto& vertex : graph.vertices())
            require(AutomatonGraphEditor::workspaceRect().adjusted(34, 34, -34, -34).contains(vertex.position),
                "whole vertices stay inside the visible workspace boundary");
    }
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(start), graph.viewport()->mapToGlobal(start),
        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(graph.viewport(), &release);
    require(QLineF(initial, graph.vertices()[1].position).length() > 300 && graph.edges().size() == 3,
        "far dragging moves the vertex to a boundary while retaining all attached edges");
    // Also exercise programmatic restores and moves beyond the old, unbounded scene.
    auto legacy = graph.toJson();
    auto vertices = legacy.value("vertices").toArray();
    auto first = vertices[0].toObject(); first.insert("x", 1e12); first.insert("y", -1e12); vertices[0] = first;
    legacy.insert("vertices", vertices);
    auto edges = legacy.value("edges").toArray();
    auto edge = edges[0].toObject(); edge.insert("controlX", -1e12); edge.insert("controlY", 1e12); edges[0] = edge;
    legacy.insert("edges", edges);
    require(graph.fromJson(legacy), "old off-canvas progress safely restores");
    for (const auto& vertex : graph.vertices())
        require(AutomatonGraphEditor::workspaceRect().contains(vertex.position), "restored positions are bounded");
    for (const auto& edge : graph.edges())
        require(qAbs(edge.controlOffset.x()) <= canvas.width() && qAbs(edge.controlOffset.y()) <= canvas.height(),
            "restored edge bends cannot grow an unbounded path");
    graph.centerOn(graph.vertices().first().position); QApplication::processEvents();
    if (!screenshot.isEmpty()) graph.grab().save(screenshot + ".canvas-boundary.png");
}

void testUnsavedChanges() {
    MainWindow window;
    TrainingVariant task; task.number = 7; task.reaction = {{"x9", {"y4"}}};
    auto start = [&] {
        require(MainWindowTestAccess::setup(window, task), "fresh work starts");
        window.findChild<QStackedWidget*>()->setCurrentIndex(1);
        window.show(); window.activateWindow(); QApplication::processEvents();
    };
    int questions = 0;
    auto answer = [&](QMessageBox::StandardButton choice) {
        QTimer::singleShot(0, &window, [&, choice] {
            if (auto* dialog = window.findChild<QMessageBox*>("unsavedChangesDialog")) {
                ++questions;
                dialog->button(choice)->click();
            }
        });
    };
    start();
    require(!MainWindowTestAccess::dirty(window), "new untouched work is clean");
    answer(QMessageBox::Cancel); MainWindowTestAccess::back(window); QApplication::processEvents();
    require(questions == 0 && window.findChild<QStackedWidget*>()->currentIndex() == 0,
        "returning from untouched work does not warn");
    start();
    TranslationManager::instance().setLocale("en", false); QApplication::processEvents();
    window.findChild<AutomatonEditor*>("sourceEditor")->findChild<HintSplitter*>("tableAssignment")->setExpanded(false);
    require(!MainWindowTestAccess::dirty(window), "language and hint layout changes do not mark answers modified");
    TranslationManager::instance().setLocale("ru_RU", false);
    MainWindowTestAccess::acceptCounts(window);
    QTemporaryDir temporary; const auto path = temporary.filePath("work.m2mwork");
    require(MainWindowTestAccess::save(window, path) && !MainWindowTestAccess::dirty(window), "successful save establishes a clean baseline");
    auto* table = window.findChild<AutomatonEditor*>("sourceEditor")->findChild<QTableWidget*>("transitionTable");
    table->editItem(table->item(4, 0)); QApplication::processEvents();
    auto* edit = table->findChild<QLineEdit*>(); require(edit, "pending cell editor opens");
    edit->setText("x9"); edit->setFocus();
    answer(QMessageBox::Cancel); require(!window.close(), "Cancel prevents closing dirty work"); QApplication::processEvents();
    require(questions == 1 && table->item(4, 0)->text() == "x9" && MainWindowTestAccess::dirty(window),
        "closing commits the active cell before checking unsaved changes");
    answer(QMessageBox::Cancel); MainWindowTestAccess::back(window); QApplication::processEvents();
    require(questions == 2 && window.findChild<QStackedWidget*>()->currentIndex() == 1,
        "Cancel also prevents returning to variant selection");
    answer(QMessageBox::Save); require(window.close(), "Save writes work before closing"); QApplication::processEvents();
    QJsonObject saved; require(ProgressStore::read(path, &saved).ok() && !MainWindowTestAccess::dirty(window), "saved work is clean");
    require(saved.value("source").toObject().value("table").toArray()[4].toArray()[0].toString() == "x9",
        "the pending cell is present in the saved file");
    window.show(); QApplication::processEvents();
    answer(QMessageBox::Cancel); MainWindowTestAccess::back(window); QApplication::processEvents();
    require(questions == 3 && window.findChild<QStackedWidget*>()->currentIndex() == 0,
        "saved work returns without an extra warning");
    start(); MainWindowTestAccess::acceptCounts(window);
    require(!MainWindowTestAccess::save(window, temporary.filePath("missing/work.m2mwork")), "unwritable destination returns failure");
    answer(QMessageBox::Save); require(!window.close(), "save failure prevents closing"); QApplication::processEvents();
    require(MainWindowTestAccess::dirty(window), "failed save retains the unsaved state");
    answer(QMessageBox::Discard); MainWindowTestAccess::back(window); QApplication::processEvents();
    require(window.findChild<QStackedWidget*>()->currentIndex() == 0, "Discard returns to selection");
    start(); MainWindowTestAccess::acceptCounts(window);
    answer(QMessageBox::Discard); require(window.close(), "Discard allows closing dirty work"); QApplication::processEvents();
}

void testHintDrag() {
    auto* hint = new TaskImage;
    auto* work = new QWidget; work->setMinimumHeight(90);
    HintSplitter splitter("Подсказка", hint, work, "testDivider");
    splitter.resize(600, 500); splitter.show(); QApplication::processEvents();
    auto* handle = splitter.handle(1);
    const int stop = splitter.widget(0)->minimumHeight();
    auto moveTo = [&](int y) {
        const auto global = splitter.mapToGlobal(QPoint(splitter.width() / 2, y + splitter.handleWidth() / 2));
        QTest::mouseMove(handle, handle->mapFromGlobal(global), 20);
    };
    QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, handle->rect().center());
    moveTo(stop);
    require(splitter.hintExtent() >= stop, "hint reaches its minimum stop before collapsing");
    moveTo(0);
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier, handle->rect().center());
    require(splitter.hintExtent() == 0 && handle->height() == splitter.handleWidth(), "continued drag collapses hint to the narrow divider");
    QTest::mousePress(handle, Qt::LeftButton, Qt::NoModifier, handle->rect().center());
    moveTo(180);
    QTest::mouseRelease(handle, Qt::LeftButton, Qt::NoModifier, handle->rect().center());
    require(splitter.expanded(), "dragging the remaining handle reopens the hint");
    QTest::mouseDClick(handle, Qt::LeftButton); require(!splitter.expanded(), "double click can collapse hint");
    QTest::keyClick(handle, Qt::Key_Return); require(splitter.expanded(), "keyboard can restore hint");
}

void testMenuColorsAndDividerGaps() {
    QMenuBar menu;
    menu.setNativeMenuBar(false);
    AppIcons::configureMenuBar(&menu);
    auto* file = menu.addMenu(AppIcons::icon(AppIcons::Kind::File), "File");
    menu.resize(320, 40); menu.show();
    for (const auto& text : {QColor(15, 85, 140), QColor(220, 235, 190)}) {
        auto colors = menu.palette();
        colors.setColor(QPalette::WindowText, text);
        colors.setColor(QPalette::ButtonText, QColor(175, 35, 90));
        menu.setPalette(colors); QApplication::processEvents();
        const auto image = menu.grab().toImage();
        auto caption = menu.actionGeometry(file->menuAction()).adjusted(26, 0, 0, 0);
        const auto area = QRect(qRound(caption.x() * image.devicePixelRatio()), qRound(caption.y() * image.devicePixelRatio()),
            qRound(caption.width() * image.devicePixelRatio()), qRound(caption.height() * image.devicePixelRatio())).intersected(image.rect());
        int matching = 0;
        for (int y = area.top(); y <= area.bottom(); ++y)
            for (int x = area.left(); x <= area.right(); ++x) {
                const auto pixel = image.pixelColor(x, y);
                if (qAbs(pixel.red() - text.red()) < 18 && qAbs(pixel.green() - text.green()) < 18 && qAbs(pixel.blue() - text.blue()) < 18)
                    ++matching;
            }
        require(matching > 4, "menu captions repaint using the current WindowText color");
    }
    for (const auto orientation : {Qt::Vertical, Qt::Horizontal}) {
        for (const auto& background : {QColor(42, 44, 47), QColor(245, 245, 245)}) {
            QWidget page; page.setAutoFillBackground(true);
            auto palette = page.palette(); palette.setColor(QPalette::Window, background);
            palette.setColor(QPalette::WindowText, background.lightness() > 128 ? Qt::black : Qt::white);
            page.setPalette(palette);
            auto* layout = new QVBoxLayout(&page);
            auto* split = new HintSplitter("Hint", new QWidget, new QWidget, "gapTest", orientation);
            layout->addWidget(split); page.resize(600, 400); page.show(); QApplication::processEvents();
            auto* handle = split->handle(1);
            const auto image = handle->grab().toImage();
            auto pixel = [&](QPoint point) {
                return image.pixelColor(qFloor((point.x() + 0.5) * image.devicePixelRatio()),
                    qFloor((point.y() + 0.5) * image.devicePixelRatio()));
            };
            const auto before = orientation == Qt::Vertical ? QPoint(60, 2) : QPoint(2, 60);
            const auto after = orientation == Qt::Vertical ? QPoint(60, handle->height() - 3) : QPoint(handle->width() - 3, 60);
            const auto track = orientation == Qt::Vertical ? QPoint(60, handle->rect().center().y()) : QPoint(handle->rect().center().x(), 60);
            require(pixel(before) == background && pixel(after) == background, "divider leaves genuine page-colored gaps on both sides");
            // A one-pixel line can land between logical pixels at fractional DPI.
            const int tx = qRound(track.x() * image.devicePixelRatio());
            const int ty = qRound(track.y() * image.devicePixelRatio());
            bool visibleTrack = false;
            for (int offset = -2; offset <= 2; ++offset) {
                const auto point = orientation == Qt::Vertical ? QPoint(tx, ty + offset) : QPoint(tx + offset, ty);
                if (image.rect().contains(point)) visibleTrack |= image.pixelColor(point) != background;
            }
            require(visibleTrack, "thin divider remains visible between the gaps");
        }
    }
}

void testImages() {
    QImage source(1200, 300, QImage::Format_RGB32); source.fill(Qt::red);
    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); source.save(&buffer, "PNG");
    TaskImage image; image.setImageData(bytes); image.show();
    for (const auto& size : {QSize(301, 180), QSize(901, 90), QSize(101, 500)}) {
        image.resize(size); QApplication::processEvents();
        const auto fit = image.imageRect();
        require(image.contentsRect().contains(fit), "image always fits the available viewport");
        require(qAbs(double(fit.width()) / fit.height() - 4.0) < 0.2, "image keeps original aspect ratio");
        const auto rendered = image.grab().toImage();
        require(rendered.pixelColor(QPoint(qRound(fit.center().x() * rendered.devicePixelRatio()),
                                          qRound(fit.center().y() * rendered.devicePixelRatio()))).red() > 200, "fitted image is rendered");
    }
}

void testValidationAndFeedback() {
    MainWindow footerWindow;
    footerWindow.show(); QApplication::processEvents();
    auto* footer = footerWindow.findChild<FeedbackLabel*>("applicationNotification");
    require(footerWindow.findChild<QWidget*>("notificationArea")->isHidden(), "footer reserves no space when empty");
    footer->showMessage("Сохранено", UiFeedbackLevel::Success);
    require(!footerWindow.findChild<QWidget*>("notificationArea")->isHidden(), "shared footer becomes visible for notifications");
    QApplication::processEvents();
    const int left = footer->mapTo(&footerWindow, QPoint()).x();
    const int right = footerWindow.width() - left - footer->width();
    require(left == right, "notification has equal left and right margins");
    FeedbackLabel notice; notice.resize(500, 70); notice.show();
    notice.showMessage("Проверка", UiFeedbackLevel::Error);
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(notice.graphicsEffect());
    QTest::qWait(120); require(effect->opacity() > 0 && effect->opacity() < 1, "message fades in");
    QTest::qWait(180); require(effect->opacity() == 1, "message fully visible after 250 ms");
    notice.showMessage("Верно", UiFeedbackLevel::Success);
    QTest::qWait(4000); require(effect->opacity() == 1 && notice.text() == "Верно", "new message gets full hold duration");
    QTest::qWait(550); require(notice.text().isEmpty() && effect->opacity() == 0, "message fades out and clears");
    require(footerWindow.findChild<QWidget*>("notificationArea")->isHidden(), "expired notification frees the footer space");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyle(new ContrastStyle);
    QTemporaryDir settings;
    QCoreApplication::setOrganizationName("M2MTests"); QCoreApplication::setApplicationName("M2MConvertorTests");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    require(TranslationManager::instance().setLocale("ru_RU", false).ok(), "Russian translation resource loads");
    QPalette dark = app.palette();
    dark.setColor(QPalette::Window, QColor(32, 34, 37));
    dark.setColor(QPalette::WindowText, Qt::white);
    dark.setColor(QPalette::Base, QColor(24, 26, 29));
    dark.setColor(QPalette::AlternateBase, QColor(42, 45, 49));
    dark.setColor(QPalette::Text, Qt::white);
    dark.setColor(QPalette::Button, QColor(48, 51, 56));
    dark.setColor(QPalette::ButtonText, Qt::white);
    app.setPalette(dark);
    MainWindow window;
    window.show();
    app.processEvents();
    require(window.screen()->availableGeometry().contains(window.frameGeometry()),
            "initial window including its frame fits the available screen");
    require(window.windowTitle().contains(QStringLiteral("M2MConvertor")),
            "application title is configured");
    require(window.styleSheet().isEmpty(), "operating-system palette is not overridden");
    auto* openArchive = window.findChild<QPushButton*>(QStringLiteral("openArchiveButton"));
    auto* begin = window.findChild<QPushButton*>(QStringLiteral("beginWorkButton"));
    auto* openProgress = window.findChild<QPushButton*>(QStringLiteral("openProgressButton"));
    require(openArchive && begin && openProgress, "landing actions are available");
    require(openArchive->styleSheet().isEmpty() && begin->styleSheet().isEmpty(),
            "buttons keep the operating-system style");
    require(!begin->isEnabled(), "work cannot start before BazaVar and variant selection");
    const auto startupSize = window.size();
    for (const auto locale : {"ru_RU", "en"}) {
        TranslationManager::instance().setLocale(locale, false);
        for (const auto& size : {QSize(900, 500), QSize(480, 360), QSize(350, 300)}) {
            window.resize(size); QApplication::processEvents(); QApplication::processEvents();
            for (auto* button : {openArchive, begin, openProgress}) requireVisibleAction(&window, button);
        }
    }
    TranslationManager::instance().setLocale("ru_RU", false);
    window.resize(startupSize); QApplication::processEvents(); QApplication::processEvents();
    auto* landingTitle = window.findChild<QLabel*>(QStringLiteral("landingTitle"));
    require(landingTitle && landingTitle->palette().color(QPalette::WindowText) == Qt::white,
            "labels inherit a dark operating-system palette");
    auto* landingContent = window.findChild<QWidget*>(QStringLiteral("landingContent"));
    require(landingContent && landingContent->width() > window.width() * 0.8 &&
            landingContent->height() > 0,
            "landing content uses most of the main window");
    for (auto* label : window.findChild<QWidget*>(QStringLiteral("landingContent"))->findChildren<QLabel*>())
        if (label->wordWrap()) require(label->height() >= label->heightForWidth(label->width()) - 1,
            "landing instructions fit at the startup minimum size");
    auto* workflow = window.findChild<QTabWidget*>(QStringLiteral("workflowTabs"));
    require(workflow && workflow->count() == 4, "four-stage workflow is created");
    require(workflow->tabText(0) == QString::fromUtf8("Начало работы"),
            "second specification screen is named Start work");
    require(!workflow->isTabEnabled(1) && !workflow->isTabEnabled(2) &&
            !workflow->isTabEnabled(3), "later stages are initially locked");
    require(window.findChild<QPushButton*>(QStringLiteral("createVertexButton")) &&
            window.findChild<QPushButton*>(QStringLiteral("deleteVertexButton")),
            "graph page exposes the two requested vertex commands");

    Automaton graphTemplate;
    graphTemplate.kind = AutomatonKind::Mealy;
    graphTemplate.states = {QStringLiteral("q1"), QStringLiteral("q2")};
    graphTemplate.inputs = {QStringLiteral("x1")};
    graphTemplate.outputs = {QStringLiteral("y1")};
    graphTemplate.initialState = 0;
    graphTemplate.transitions = {{0, 0, 1, {0}}, {1, 0, 0, {0}}};
    AutomatonGraphEditor graphEditor;
    graphEditor.resize(760, 520);
    graphEditor.setTemplate(graphTemplate);
    graphEditor.show();
    app.processEvents();
    graphEditor.createVertex();
    graphEditor.createVertex();
    require(graphEditor.vertices().size() == 2 &&
            graphEditor.vertices()[0].label == QStringLiteral("a0") &&
            graphEditor.vertices()[1].label == QStringLiteral("a1"),
            "vertex command creates movable suggested states");
    const QPointF initialFirstPosition = graphEditor.vertices()[0].position;
    const QPoint firstPoint = graphEditor.mapFromScene(graphEditor.vertices()[0].position);
    const QPoint secondPoint = graphEditor.mapFromScene(graphEditor.vertices()[1].position);
    QTest::mouseClick(graphEditor.viewport(), Qt::LeftButton, Qt::NoModifier, firstPoint);
    QTest::mouseClick(graphEditor.viewport(), Qt::LeftButton, Qt::ControlModifier, secondPoint);
    require(graphEditor.edges().size() == 1,
            "Ctrl-click on a second vertex creates an edge");

    const QPoint dragTarget = firstPoint + QPoint(85, 55);
    QTest::mousePress(graphEditor.viewport(), Qt::LeftButton, Qt::NoModifier, firstPoint);
    QTest::mouseMove(graphEditor.viewport(), dragTarget, 20);
    QTest::mouseRelease(graphEditor.viewport(), Qt::LeftButton, Qt::NoModifier, dragTarget);
    require(QLineF(graphEditor.vertices()[0].position, initialFirstPosition).length() > 20,
            "vertices can be dragged and edges follow their positions");

    const QVector<GraphVertexData> movedVertices = graphEditor.vertices();
    const QPointF edgeMiddle = (movedVertices[0].position + movedVertices[1].position) / 2.0;
    const QPoint middlePoint = graphEditor.mapFromScene(edgeMiddle);
    const QPoint bendTarget = middlePoint + QPoint(0, 70);
    QTest::mousePress(graphEditor.viewport(), Qt::LeftButton, Qt::NoModifier, middlePoint);
    QTest::mouseMove(graphEditor.viewport(), bendTarget, 20);
    QTest::mouseRelease(graphEditor.viewport(), Qt::LeftButton, Qt::NoModifier, bendTarget);
    require(QLineF(QPointF(), graphEditor.edges()[0].controlOffset).length() > 20,
            "an edge can be dragged away from its automatic curve");

    AutomatonGraphEditor restoredGraph;
    restoredGraph.setTemplate(graphTemplate);
    QString graphError;
    QJsonObject savedGraph = graphEditor.toJson();
    QJsonArray savedEdges = savedGraph.value(QStringLiteral("edges")).toArray();
    QJsonObject savedEdge = savedEdges[0].toObject();
    savedEdge.insert(QStringLiteral("label"), QStringLiteral("x1 / y1"));
    savedEdges[0] = savedEdge;
    savedGraph.insert(QStringLiteral("edges"), savedEdges);
    require(restoredGraph.fromJson(savedGraph, &graphError) &&
            restoredGraph.vertices().size() == 2 && restoredGraph.edges().size() == 1 &&
            restoredGraph.edges()[0].label == QStringLiteral("x1 / y1") &&
            QLineF(QPointF(), restoredGraph.edges()[0].controlOffset).length() > 20,
            "labels, vertex positions and manual edge curves survive progress serialization");

    const QPoint movedFirstPoint = graphEditor.mapFromScene(graphEditor.vertices()[0].position);
    QTest::mouseClick(graphEditor.viewport(), Qt::LeftButton, Qt::NoModifier, movedFirstPoint);
    QTest::keyClick(graphEditor.viewport(), Qt::Key_Backspace);
    require(graphEditor.vertices().size() == 1 && graphEditor.edges().isEmpty(),
            "Backspace removes a selected vertex and its edges");

    Automaton mooreTemplate;
    mooreTemplate.kind = AutomatonKind::Moore;
    mooreTemplate.states = {QStringLiteral("s1")};
    mooreTemplate.inputs = {QStringLiteral("x1")};
    mooreTemplate.outputs = {QStringLiteral("y1")};
    mooreTemplate.initialState = 0;
    mooreTemplate.mooreOutputs = {{0}};
    mooreTemplate.transitions = {{0, 0, 0, {}}};
    AutomatonGraphEditor loopEditor;
    loopEditor.resize(500, 400);
    loopEditor.setTemplate(mooreTemplate);
    loopEditor.show();
    app.processEvents();
    loopEditor.createVertex();
    require(loopEditor.vertices()[0].label == QStringLiteral("a0 / -"),
            "Moore vertices receive state/output labels");
    const QPoint loopPoint = loopEditor.mapFromScene(loopEditor.vertices()[0].position);
    QTimer::singleShot(0, [] {
        for (auto* widget : QApplication::topLevelWidgets()) {
            if (auto* dialog = qobject_cast<QInputDialog*>(widget)) {
                auto* field = dialog->findChild<QLineEdit*>();
                require(field && !field->validator(), "graph label field has no typing validator");
                field->setText("bad @ output"); dialog->accept();
            }
        }
    });
    QTest::mouseDClick(loopEditor.viewport(), Qt::LeftButton, Qt::NoModifier, loopPoint);
    require(loopEditor.vertices()[0].label == "a0 / bad @ output", "graph labels retain raw invalid drafts");
    QTest::mouseClick(loopEditor.viewport(), Qt::LeftButton, Qt::NoModifier, loopPoint);
    QTest::mouseClick(loopEditor.viewport(), Qt::LeftButton, Qt::ControlModifier, loopPoint);
    require(loopEditor.edges().size() == 1 &&
            loopEditor.edges()[0].fromId == loopEditor.edges()[0].toId,
            "Ctrl-click on the selected vertex creates a self-loop");
    if (argc > 1) {
        require(window.grab().save(QString::fromLocal8Bit(argv[1])),
                "GUI smoke screenshot is saved");
        auto* pages = window.findChild<QStackedWidget*>();
        require(pages, "main page stack is available");
        pages->setCurrentIndex(1);
        app.processEvents();
        require(window.grab().save(QString::fromLocal8Bit(argv[1]) + QStringLiteral(".workspace.png")),
                "workspace GUI smoke screenshot is saved");
        restoredGraph.resize(760, 520);
        restoredGraph.show();
        app.processEvents();
        require(restoredGraph.grab().save(
                    QString::fromLocal8Bit(argv[1]) + QStringLiteral(".graph.png")),
                "interactive graph screenshot is saved");
    }
    testReactionWorkflow(window, argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString());
    testBoundedGraph(argc > 1 ? QString::fromLocal8Bit(argv[1]) : QString());
    testUnsavedChanges();
    testJournal();
    testHintDrag();
    testMenuColorsAndDividerGaps();
    testImages();
    testValidationAndFeedback();
    std::cout << "PASS GUI startup, workflow gates and interactive graph editor\n";
    return 0;
}
