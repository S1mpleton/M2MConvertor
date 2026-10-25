#include "mainwindow.h"
#include "ui/automatongrapheditor.h"

#include <QApplication>
#include <QLabel>
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
    std::cout << "PASS GUI startup, workflow gates and interactive graph editor\n";
    return 0;
}
