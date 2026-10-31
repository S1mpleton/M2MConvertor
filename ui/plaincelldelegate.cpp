#include "ui/plaincelldelegate.h"
#include <QLineEdit>
#include <QPainter>
#include "ui/contraststyle.h"
void PlainCellDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
    QStyledItemDelegate::paint(painter, option, index);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(TableStyle::borderColor(option.palette), TableStyle::borderWidth));
    const auto rect = QRectF(option.rect).adjusted(0, 0, -0.5, -0.5);
    painter->drawLine(rect.topRight(), rect.bottomRight());
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    painter->restore();
}
void PlainCellDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                     const QModelIndex& index) const {
    if (auto* field = qobject_cast<QLineEdit*>(editor)) {
        model->setData(index,
                       field->text().trimmed().isEmpty() ? QStringLiteral("-") : field->text(),
                       Qt::EditRole);
    } else
        QStyledItemDelegate::setModelData(editor, model, index);
}
