#include "domain/result.h"
QString resultLevelName(ResultLevel level) {
    switch (level) {
    case ResultLevel::Info:
        return QCoreApplication::translate("Result", "Информация");
    case ResultLevel::Warning:
        return QCoreApplication::translate("Result", "Предупреждение");
    case ResultLevel::Error:
        return QCoreApplication::translate("Result", "Ошибка");
    }
    return {};
}
QString resultCategoryName(ResultCategory category) {
    switch (category) {
    case ResultCategory::Application:
        return QCoreApplication::translate("Result", "Приложение");
    case ResultCategory::Archive:
        return QCoreApplication::translate("Result", "Архив");
    case ResultCategory::Progress:
        return QCoreApplication::translate("Result", "Работа");
    case ResultCategory::Validation:
        return QCoreApplication::translate("Result", "Проверка");
    case ResultCategory::Report:
        return QCoreApplication::translate("Result", "Отчёт");
    case ResultCategory::Translation:
        return QCoreApplication::translate("Result", "Перевод");
    }
    return {};
}
