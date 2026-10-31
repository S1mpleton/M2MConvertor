#pragma once
#include "domain/result.h"
#include <QObject>
#include <QTranslator>
#include <memory>

class TranslationManager : public QObject {
    Q_OBJECT
  public:
    static TranslationManager& instance();
    QString locale() const {
        return m_locale;
    }
    Result setLocale(const QString& locale, bool persist = true);
    Result loadPreference();
  signals:
    void languageChanged();

  private:
    TranslationManager() = default;
    QString m_locale;
    std::unique_ptr<QTranslator> m_app;
    std::unique_ptr<QTranslator> m_qt;
};
