#include "mainwindow.h"
#include "ui/automatongrapheditor.h"
#include "ui/automatoneditor.h"
#include "ui/mealycelldelegate.h"
#include "ui/widgets/taskimage.h"
#include "ui/widgets/collapsiblepanel.h"
#include <QStatusBar>
#include <QBuffer>
#include <QToolButton>
#include "ui/widgets/feedbacklabel.h"
#include <QGraphicsOpacityEffect>

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
    table->editItem(table->item(4, 0)); QApplication::processEvents();
    auto* freeInput = table->findChild<QLineEdit*>();
    require(freeInput && !freeInput->validator(), "Moore input has no typing validator");
    QTest::keyClicks(freeInput, "bad @ input"); QTest::keyClick(freeInput, Qt::Key_Return); QApplication::processEvents();
    require(table->item(4, 0)->text() == "bad @ input", "invalid drafts are saved unchanged");
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(!source->toJson().value("tableAccepted").toBool() &&
        window.findChild<FeedbackLabel*>("applicationNotification")->text().contains("x0"),
        "invalid input is reported on Accept in the shared footer");
    table->item(1, 1)->setText("y4"); table->item(4, 0)->setText("x9");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".table.png");
    }
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(source->toJson().value("tableAccepted").toBool(), "screenshot's table without a transition is accepted");
    table->item(4, 1)->setText("a0");
    source->findChild<QPushButton*>("acceptTableButton")->click();
    require(source->toJson().value("tableAccepted").toBool(), "table with final loop is also accepted");
    auto* matrix = source->findChild<QTableWidget*>("transitionMatrix");
    auto* outputs = source->findChild<QTableWidget*>("matrixOutputs");
    require(matrix->item(0, 0)->text() == "-", "matrix defaults to no transitions");
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
    auto* toggle = source->findChild<QToolButton*>("graphHintToggle");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents(); window.grab().save(screenshot + ".graph-reference.png");
    }
    toggle->click(); QApplication::processEvents();
    require(graphReference->isHidden() && source->completed(), "hiding graph reference keeps acceptance");
    auto* t = target->findChild<QTableWidget*>("transitionTable");
    require(t->item(2, 1)->text() == "-/-", "Mealy transitions default to -/-");
    workflow->setCurrentIndex(2); QApplication::processEvents();
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
    target->findChild<QTableWidget*>("transitionMatrix")->item(0, 0)->setText("x9/y4");
    target->findChild<QPushButton*>("acceptMatrixButton")->click();
    auto* g = target->findChild<AutomatonGraphEditor*>(); g->createVertex();
    auto gd = g->toJson(); gd.insert("edges", QJsonArray{QJsonObject{{"from", 1}, {"to", 1}, {"label", "x9/y4"}}});
    require(g->fromJson(gd, &error), "Mealy graph restores");
    target->findChild<QPushButton*>("acceptGraphButton")->click();
    require(target->completed() && workflow->isTabEnabled(3), "six accepted representations unlock report");
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
    MainWindow restored;
    require(MainWindowTestAccess::restore(restored, MainWindowTestAccess::progress(window)), "progress reloads");
    require(restored.findChild<AutomatonEditor*>("sourceEditor")->completed() &&
        restored.findChild<AutomatonEditor*>("targetEditor")->completed(), "new table and flexible graph revalidate after reload");
    require(!restored.findChild<AutomatonEditor*>("sourceEditor")->findChild<CollapsiblePanel*>("graphHint")->expanded(),
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
    require(footerWindow.statusBar()->isHidden(), "footer reserves no space when empty");
    footer->showMessage("Сохранено", UiFeedbackLevel::Success);
    require(!footerWindow.statusBar()->isHidden(), "shared footer becomes visible for notifications");
    FeedbackLabel notice; notice.resize(500, 70); notice.show();
    notice.showMessage("Проверка", UiFeedbackLevel::Error);
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(notice.graphicsEffect());
    QTest::qWait(120); require(effect->opacity() > 0 && effect->opacity() < 1, "message fades in");
    QTest::qWait(180); require(effect->opacity() == 1, "message fully visible after 250 ms");
    notice.showMessage("Верно", UiFeedbackLevel::Success);
    QTest::qWait(4000); require(effect->opacity() == 1 && notice.text() == "Верно", "new message gets full hold duration");
    QTest::qWait(550); require(notice.text().isEmpty() && effect->opacity() == 0, "message fades out and clears");
    require(footerWindow.statusBar()->isHidden(), "expired notification frees the footer space");
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
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
    auto* landingTitle = window.findChild<QLabel*>(QStringLiteral("landingTitle"));
    require(landingTitle && landingTitle->palette().color(QPalette::WindowText) == Qt::white,
            "labels inherit a dark operating-system palette");
    auto* landingContent = window.findChild<QWidget*>(QStringLiteral("landingContent"));
    require(landingContent && landingContent->width() > window.width() * 0.8 &&
            landingContent->height() > window.height() * 0.45,
            "landing content uses most of the main window");
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
    testImages();
    testValidationAndFeedback();
    std::cout << "PASS GUI startup, workflow gates and interactive graph editor\n";
    return 0;
}
