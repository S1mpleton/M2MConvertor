#include "infrastructure/translationmanager.h"
#include "infrastructure/logger.h"
#include "mainwindow.h"
#include "ui/contraststyle.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(new ContrastStyle);
    QCoreApplication::setOrganizationName(QStringLiteral("M2M"));
    QCoreApplication::setApplicationName(QStringLiteral("M2MConvertor"));

    Logger::instance().log(TranslationManager::instance().loadPreference());
    MainWindow w;
    w.show();
    return QApplication::exec();
}
