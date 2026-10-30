#include "ui/mealycelldelegate.h"
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>

QWidget* MealyCellDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                         const QModelIndex& index) const {
    if (index.column() == 0)
        return QStyledItemDelegate::createEditor(parent, option, index);
    auto* editor = new QWidget(parent);
    editor->setAutoFillBackground(true);
    editor->setObjectName("mealyCellEditor");
    auto* layout = new QHBoxLayout(editor);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    auto* state = new QLineEdit(editor);
    auto* outputs = new QLineEdit(editor);
    state->setObjectName("destinationEditor");
    outputs->setObjectName("outputsEditor");
    state->setPlaceholderText("-");
    outputs->setPlaceholderText("-");
    state->setMinimumWidth(0);
    outputs->setMinimumWidth(0);
    state->setToolTip(tr("Следующее состояние: одно a0, a1, … или -"));
    outputs->setToolTip(tr("Выходы: y0…y9 через запятую или -"));
    layout->addWidget(state, 1);
    layout->addWidget(new QLabel("/", editor));
    layout->addWidget(outputs, 2);
    editor->setFocusProxy(state);
    // Let both fields keep arbitrary text, including incomplete or invalid drafts.
    state->installEventFilter(const_cast<MealyCellDelegate*>(this));
    outputs->installEventFilter(const_cast<MealyCellDelegate*>(this));
    return editor;
}
void MealyCellDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    auto* state = editor->findChild<QLineEdit*>("destinationEditor");
    if (!state) {
        QStyledItemDelegate::setEditorData(editor, index);
        return;
    }
    const auto text = index.data(Qt::EditRole).toString();
    const int slash = text.indexOf('/');
    state->setText(slash < 0 ? text : text.left(slash));
    editor->findChild<QLineEdit*>("outputsEditor")->setText(slash < 0 ? "-" : text.mid(slash + 1));
    state->selectAll();
}
void MealyCellDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                     const QModelIndex& index) const {
    auto* state = editor->findChild<QLineEdit*>("destinationEditor");
    if (!state) {
        PlainCellDelegate::setModelData(editor, model, index);
        return;
    }
    const auto value = [](QLineEdit* field) {
        return field->text().trimmed().isEmpty() ? QStringLiteral("-") : field->text();
    };
    model->setData(index, value(state) + "/" + value(editor->findChild<QLineEdit*>("outputsEditor")), Qt::EditRole);
}
void MealyCellDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                             const QModelIndex& index) const {
    if (index.column() == 0)
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    else
        editor->setGeometry(option.rect);
}
bool MealyCellDelegate::eventFilter(QObject* object, QEvent* event) {
    // Child line edits own Enter/Escape/focus handling for the composite widget.
    if (object->objectName() == "mealyCellEditor")
        return false;
    auto* edit = qobject_cast<QLineEdit*>(object);
    if (edit && edit->parentWidget() && edit->parentWidget()->objectName() == "mealyCellEditor") {
        auto* editor = edit->parentWidget();
        if (editor->property("committing").toBool())
            return false;
        if (event->type() == QEvent::KeyPress) {
            const int key = static_cast<QKeyEvent*>(event)->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter) {
                editor->setProperty("committing", true);
                emit commitData(editor);
                emit closeEditor(editor);
                return true;
            }
            if (key == Qt::Key_Escape) {
                editor->setProperty("committing", true);
                emit closeEditor(editor, RevertModelCache);
                return true;
            }
        }
        if (event->type() == QEvent::FocusOut) {
            auto* focus = QApplication::focusWidget();
            if (focus != editor && (!focus || !editor->isAncestorOf(focus))) {
                editor->setProperty("committing", true);
                emit commitData(editor);
                emit closeEditor(editor);
            }
        }
        // Switching between the two fields keeps the composite editor open.
        return false;
    }
    return QStyledItemDelegate::eventFilter(object, event);
}
