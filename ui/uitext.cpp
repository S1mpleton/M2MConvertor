#include "ui/uitext.h"
#include "ui/widgets/wrappinglabel.h"
void UiText::refresh(QObject* root) {
    for (const auto& key : root->dynamicPropertyNames()) {
        if (key.startsWith("i18nTab_")) {
            if (auto* tabs = qobject_cast<QTabWidget*>(root))
                tabs->setTabText(key.mid(8).toInt(),
                                 text(root->property(key.constData()).toByteArray().constData()));
        }
        if (key.startsWith("i18n_"))
            root->setProperty(key.mid(5).constData(),
                              text(root->property(key.constData()).toByteArray().constData()));
    }
    if (auto* label = qobject_cast<WrappingLabel*>(root))
        label->refreshHeight();
    for (auto* child : root->children())
        refresh(child);
}
