#include "mainwindow.h"

#include <QApplication>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>

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
    if (argc > 1) {
        require(window.grab().save(QString::fromLocal8Bit(argv[1])),
                "GUI smoke screenshot is saved");
        auto* pages = window.findChild<QStackedWidget*>();
        require(pages, "main page stack is available");
        pages->setCurrentIndex(1);
        app.processEvents();
        require(window.grab().save(QString::fromLocal8Bit(argv[1]) + QStringLiteral(".workspace.png")),
                "workspace GUI smoke screenshot is saved");
    }
    std::cout << "PASS GUI startup and workflow gates\n";
    return 0;
}
