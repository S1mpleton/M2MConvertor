#pragma once
#include <QCoreApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QTabWidget>
#include <QVariant>
#include <type_traits>

// Bind only application-owned captions; never inspect or replace user input text.
namespace UiText {
inline QString text(const char* source) {
    return QCoreApplication::translate("App", source);
}
inline void bind(QObject* object, const char* property, const char* source) {
    object->setProperty((QByteArray("i18n_") + property).constData(), QByteArray(source));
    object->setProperty(property, text(source));
}
template <class T> T* make(const char* source) {
    auto* widget = new T;
    bind(widget, std::is_same_v<T, QGroupBox> ? "title" : "text", source);
    return widget;
}
inline void row(QFormLayout* layout, const char* source, QWidget* field) {
    auto* label = new QLabel;
    bind(label, "text", source);
    layout->addRow(label, field);
}
inline QAction* action(QMenu* menu, const QIcon& icon, const char* source) {
    auto* action = menu->addAction(icon, text(source));
    bind(action, "text", source);
    return action;
}
inline QMenu* menu(QMenuBar* bar, const QIcon& icon, const char* source) {
    auto* menu = bar->addMenu(icon, text(source));
    bind(menu, "title", source);
    return menu;
}
inline void tab(QTabWidget* tabs, QWidget* page, const char* source) {
    const int index = tabs->addTab(page, text(source));
    tabs->setProperty(("i18nTab_" + QByteArray::number(index)).constData(), QByteArray(source));
}
void refresh(QObject* root);
} // namespace UiText
