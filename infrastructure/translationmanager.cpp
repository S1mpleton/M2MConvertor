#include "infrastructure/translationmanager.h"
#include <QCoreApplication>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
TranslationManager& TranslationManager::instance() {
    static TranslationManager manager;
    return manager;
}
Result TranslationManager::setLocale(const QString& locale, bool persist) {
    if (locale != "ru_RU" && locale != "en")
        return Result::error(
            Message(QT_TRANSLATE_NOOP("Result", "Язык %1 не поддерживается.")).arg(locale),
            ResultCategory::Translation);
    if (locale == m_locale)
        return Result::success();
    auto next = std::make_unique<QTranslator>();
    if (!next->load(":/i18n/M2MConvertor_" + locale + ".qm") || next->isEmpty())
        return Result::error(
            Message(QT_TRANSLATE_NOOP("Result", "Не удалось загрузить перевод %1.")).arg(locale),
            ResultCategory::Translation);
    if (m_app)
        QCoreApplication::removeTranslator(m_app.get());
    if (m_qt)
        QCoreApplication::removeTranslator(m_qt.get());
    m_app = std::move(next);
    QCoreApplication::installTranslator(m_app.get());
    m_qt = std::make_unique<QTranslator>();
    if (m_qt->load("qtbase_" + locale, QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        QCoreApplication::installTranslator(m_qt.get());
    m_locale = locale;
    if (persist)
        QSettings().setValue("language", locale);
    emit languageChanged();
    return Result::success(
        Message(QT_TRANSLATE_NOOP("Result", "Язык интерфейса изменён: %1.")).arg(locale),
        ResultCategory::Translation);
}
Result TranslationManager::loadPreference() {
    const auto fallback = QLocale::system().language() == QLocale::Russian ? "ru_RU" : "en";
    return setLocale(QSettings().value("language", fallback).toString(), false);
}
