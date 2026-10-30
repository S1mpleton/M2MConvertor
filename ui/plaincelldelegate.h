#pragma once
#include <QStyledItemDelegate>

// Preserve arbitrary drafts, replacing only empty/whitespace-only cells on commit.
class PlainCellDelegate : public QStyledItemDelegate {
  public:
    explicit PlainCellDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const override;
};
