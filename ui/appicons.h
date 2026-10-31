#pragma once
#include <QIcon>
class QMenuBar;
namespace AppIcons {
enum class Kind {
    Accept,
    Back,
    File,
    Settings,
    New,
    Save,
    Open,
    Image,
    Import,
    Export,
    Exit,
    Language,
    English,
    Russian,
    Log,
    Add,
    Remove,
    Swap,
    Copy,
    Clear
};
QIcon icon(Kind kind);
QIcon applicationIcon();
void configureMenuBar(QMenuBar *bar);
} // namespace AppIcons
