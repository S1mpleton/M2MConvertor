#include "ui/plaincelldelegate.h"
#include <QLineEdit>
void PlainCellDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                     const QModelIndex& index) const {
    if (auto* field = qobject_cast<QLineEdit*>(editor)) {
        model->setData(index,
                       field->text().trimmed().isEmpty() ? QStringLiteral("-") : field->text(),
                       Qt::EditRole);
    } else
        QStyledItemDelegate::setModelData(editor, model, index);
}
