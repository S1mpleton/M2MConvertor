#include "mainwindow.h"
#include "ui/automatongrapheditor.h"
#include "ui/automatoneditor.h"

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
        window.m_stateCount->setValue(3);
        window.m_inputCount->setValue(2);
        window.m_outputCount->setValue(3);
        window.acceptCounts();
    }
    static QJsonObject progress(MainWindow& window) { return window.progressData(); }
    static bool restore(MainWindow& window, const QJsonObject& data) {
        return window.restoreProgress(data, QString());
    }
};

void testReactionWorkflow(MainWindow& window, const QString& screenshot) {
    QTemporaryDir temporary;
    const auto archivePath = temporary.filePath("variants.m2m");
    auto blocked = trainingVariantJson();
    blocked.insert("number", 1);
    blocked.insert("type", "MealyToMoore");
    auto root = trainingArchiveJson();
    root.insert("variants", QJsonArray{blocked, trainingVariantJson()});
    QFile file(archivePath);
    require(file.open(QIODevice::WriteOnly), "archive file opens");
    file.write(makeCompatibleArchive(root));
    file.close();
    require(MainWindowTestAccess::open(window, archivePath), "new archive opens in main window");
    auto* combo = window.findChild<QComboBox*>("variantCombo");
    auto* begin = window.findChild<QPushButton*>("beginWorkButton");
    require(combo->currentData().toInt() == 7 && begin->isEnabled(),
            "first supported task is selected automatically");
    require(!(combo->model()->flags(combo->model()->index(0, 0)) & Qt::ItemIsEnabled),
            "Mealy-to-Moore task is disabled in selection");
    auto blockedTask = trainingVariant();
    blockedTask.direction = ConversionDirection::MealyToMoore;
    require(!MainWindowTestAccess::setup(window, blockedTask),
            "direct workspace setup cannot bypass direction gate");
    MainWindowTestAccess::begin(window);
    MainWindowTestAccess::acceptCounts(window);
    auto* source = window.findChild<AutomatonEditor*>("sourceEditor");
    auto* target = window.findChild<AutomatonEditor*>("targetEditor");
    auto* workflow = window.findChild<QTabWidget*>("workflowTabs");
    require(workflow->isTabEnabled(1) && !workflow->isTabEnabled(2),
            "correct source counts unlock only the Moore editor");
    require(source->findChild<QLineEdit*>("generatedStates")->isReadOnly() &&
            source->findChild<QLineEdit*>("generatedStates")->text() == "a0, a1, a2" &&
            target->findChild<QLineEdit*>("generatedStates")->text() == "a0, a1",
            "each editor generates the exact separate state count");
    auto* inputs = source->findChild<QLineEdit*>("inputSignals");
    auto* configure = source->findChild<QPushButton*>("configureTableButton");
    inputs->setText("x10, x2");
    configure->click();
    auto* table = source->findChild<QTableWidget*>("transitionTable");
    require(table->columnCount() == 0, "invalid input names do not create a table");
    inputs->setText("x1, x2");
    configure->click();
    require(table->rowCount() == 3 && table->columnCount() == 3,
            "Moore table has output row and generated state columns");
    table->item(0, 0)->setText("y2");
    table->item(0, 1)->setText("y0, y3");
    table->item(0, 2)->setText("-");
    table->item(1, 1)->setText("a0");
    table->item(1, 2)->setText("a0");
    table->item(2, 0)->setText("a1");
    table->item(2, 1)->setText("a2");
    auto* acceptTable = source->findChild<QPushButton*>("acceptTableButton");
    table->item(0, 1)->setText("y0, y0");
    acceptTable->click();
    require(!source->toJson().value("tableAccepted").toBool(), "duplicate outputs cannot be accepted");
    table->item(0, 1)->setText("y0, y3");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents();
        require(window.grab().save(screenshot + ".table.png"), "Moore table screenshot saves");
    }
    acceptTable->click();
    require(source->toJson().value("tableAccepted").toBool(), "reaction-valid Moore table is accepted");
    auto* matrix = source->findChild<QTableWidget*>("transitionMatrix");
    auto* outputs = source->findChild<QTableWidget*>("matrixOutputs");
    require(matrix->rowCount() == 3 && matrix->columnCount() == 3 &&
            matrix->horizontalHeaderItem(0)->text() == "a0" &&
            matrix->verticalHeaderItem(2)->text() == "a2" &&
            outputs->rowCount() == 3 && outputs->columnCount() == 2 &&
            !(outputs->item(0, 0)->flags() & Qt::ItemIsEditable) &&
            outputs->item(0, 1)->text().isEmpty(),
            "matrix has fixed state axes and independent empty outputs table");
    matrix->item(0, 1)->setText("x2");
    matrix->item(1, 0)->setText("x1");
    matrix->item(1, 2)->setText("x2");
    matrix->item(2, 0)->setText("x1");
    outputs->item(0, 1)->setText("y2");
    outputs->item(1, 1)->setText("y3");
    outputs->item(2, 1)->setText("y0");
    source->findChild<QPushButton*>("acceptMatrixButton")->click();
    require(!source->toJson().value("matrixAccepted").toBool(),
            "wrong matrix outputs are rejected even with correct distinct counts");
    outputs->item(1, 1)->setText("y3, y0");
    outputs->item(2, 1)->setText("-");
    if (!screenshot.isEmpty()) {
        QApplication::processEvents();
        require(window.grab().save(screenshot + ".matrix.png"), "Moore matrix screenshot saves");
        const auto dark = QApplication::palette();
        QApplication::setPalette(QApplication::style()->standardPalette());
        QApplication::processEvents();
        require(window.grab().save(screenshot + ".matrix-light.png"), "light matrix screenshot saves");
        QApplication::setPalette(dark);
    }
    source->findChild<QPushButton*>("acceptMatrixButton")->click();
    const auto acceptedMatrix = source->toJson().value("matrix");
    acceptTable->click();
    require(source->toJson().value("matrix") == acceptedMatrix &&
            source->toJson().value("matrixAccepted").toBool(),
            "reaccepting an unchanged table preserves the completed matrix");
    require(source->toJson().value("matrixAccepted").toBool(), "matrix is independently checked by reaction");
    auto* graph = source->findChild<AutomatonGraphEditor*>();
    for (int i = 0; i < 4; ++i) graph->createVertex();
    require(graph->vertices().size() == 3 && graph->vertices()[0].label == "a0 / -",
            "graph generates only the task's state names without supplying answers");
    auto graphData = graph->toJson();
    auto vertices = graphData.value("vertices").toArray();
    const QStringList labels = {"a0 / y2", "a1 / y3,y0", "a2 / -"};
    for (int i = 0; i < vertices.size(); ++i) {
        auto vertex = vertices[i].toObject();
        vertex.insert("label", labels[i]);
        vertices[i] = vertex;
    }
    graphData.insert("vertices", vertices);
    auto edge = [](int from, int to, const QString& label) {
        return QJsonObject{{"from", from}, {"to", to}, {"label", label}, {"controlX", 0}, {"controlY", 0}};
    };
    QJsonArray edges{edge(1, 2, "x2"), edge(2, 1, "x1"), edge(2, 3, "x2"), edge(3, 1, "x1")};
    graphData.insert("edges", edges);
    QString error;
    require(graph->fromJson(graphData, &error), "graph data loads");
    auto* acceptGraph = source->findChild<QPushButton*>("acceptGraphButton");
    auto wrongGraph = graphData;
    auto wrongEdges = edges;
    wrongEdges[0] = edge(1, 1, "x2");
    wrongGraph.insert("edges", wrongEdges);
    require(graph->fromJson(wrongGraph, &error), "incorrect graph is editable");
    acceptGraph->click();
    require(!source->completed() && source->findChild<QLabel*>("graphFeedback")->text().contains("Такт 1"),
            "graph topology is checked by its reaction, with the first mismatch reported");
    require(graph->fromJson(graphData, &error), "correct graph restores");
    acceptGraph->click();
    require(source->completed() && workflow->isTabEnabled(2),
            "reaction-valid Moore graph unlocks Mealy stage");
    target->findChild<QLineEdit*>("inputSignals")->setText("x1, x2");
    target->findChild<QPushButton*>("configureTableButton")->click();
    auto* targetTable = target->findChild<QTableWidget*>("transitionTable");
    require(targetTable->columnCount() == 2 && targetTable->rowCount() == 2,
            "Mealy table uses the Mealy counts, not the Moore counts");
    targetTable->item(0, 1)->setText("a0 / y2");
    targetTable->item(1, 0)->setText("a1 / y0,y3");
    targetTable->item(1, 1)->setText("a1 / -");
    target->findChild<QPushButton*>("acceptTableButton")->click();
    require(target->toJson().value("tableAccepted").toBool(), "Mealy table matches the same reaction");
    auto* targetMatrix = target->findChild<QTableWidget*>("transitionMatrix");
    require(target->findChild<QTableWidget*>("matrixOutputs")->isHidden(),
            "Mealy outputs belong to edges, not an output-state table");
    targetMatrix->item(0, 1)->setText("x2 / y3,y0");
    targetMatrix->item(1, 0)->setText("x1 / y2");
    targetMatrix->item(1, 1)->setText("x2 / -");
    target->findChild<QPushButton*>("acceptMatrixButton")->click();
    require(target->toJson().value("matrixAccepted").toBool(), "Mealy matrix is checked by reaction");
    auto* targetGraph = target->findChild<AutomatonGraphEditor*>();
    targetGraph->createVertex(); targetGraph->createVertex();
    auto targetGraphData = targetGraph->toJson();
    targetGraphData.insert("edges", QJsonArray{edge(1, 2, "x2 / y0,y3"),
                                               edge(2, 1, "x1 / y2"), edge(2, 2, "x2 / -")});
    require(targetGraph->fromJson(targetGraphData, &error), "Mealy graph loads");
    target->findChild<QPushButton*>("acceptGraphButton")->click();
    require(target->completed() && workflow->isTabEnabled(3), "all six representations unlock report");
    const auto progress = MainWindowTestAccess::progress(window);
    MainWindow restored;
    require(MainWindowTestAccess::restore(restored, progress), "reaction-based progress restores");
    auto* restoredSource = restored.findChild<AutomatonEditor*>("sourceEditor");
    auto* restoredTarget = restored.findChild<AutomatonEditor*>("targetEditor");
    require(restoredSource->completed() && restoredTarget->completed() &&
            restoredSource->toJson().value("matrixOutputs") == source->toJson().value("matrixOutputs"),
            "progress preserves output table and revalidates all acceptance flags");
    auto corrupted = source->toJson();
    auto matrixOutputs = corrupted.value("matrixOutputs").toArray();
    matrixOutputs[1] = QJsonArray{"a1", "y2"};
    corrupted.insert("matrixOutputs", matrixOutputs);
    restoredSource->fromJson(corrupted);
    require(!restoredSource->completed(), "forged acceptance flags cannot bypass reaction check");
    table->item(0, 1)->setText("y0");
    require(!source->completed() && !target->completed() && !workflow->isTabEnabled(2),
            "upstream edits invalidate the report and downstream results");
    auto blockedProgress = progress;
    blockedProgress.insert("variantNumber", 1);
    const auto blockedPath = temporary.filePath("blocked.m2m");
    require(QFile::copy(archivePath, blockedPath), "second archive is available");
    blockedProgress.insert("archivePath", blockedPath);
    const auto previousArchive = MainWindowTestAccess::progress(restored).value("archivePath");
    QTimer::singleShot(0, [] {
        for (auto* widget : QApplication::topLevelWidgets())
            if (auto* message = qobject_cast<QMessageBox*>(widget)) message->accept();
    });
    require(!MainWindowTestAccess::restore(restored, blockedProgress),
            "saved progress cannot open unfinished conversion");
    require(MainWindowTestAccess::progress(restored).value("archivePath") == previousArchive,
            "rejected progress does not replace the current work's archive");
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
            graphEditor.vertices()[0].label == QStringLiteral("q1") &&
            graphEditor.vertices()[1].label == QStringLiteral("q2"),
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
    require(loopEditor.vertices()[0].label == QStringLiteral("s1 / y1"),
            "Moore vertices receive state/output labels");
    const QPoint loopPoint = loopEditor.mapFromScene(loopEditor.vertices()[0].position);
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
    std::cout << "PASS GUI startup, workflow gates and interactive graph editor\n";
    return 0;
}
