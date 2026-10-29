#pragma once
#include <QStyledItemDelegate>

// Editing is unrestricted. The automaton parser checks the committed values on Accept.
class MealyCellDelegate : public QStyledItemDelegate {
  public:
    explicit MealyCellDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override;
    void setEditorData(QWidget*, const QModelIndex&) const override;
    void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const override;
    void updateEditorGeometry(QWidget*, const QStyleOptionViewItem&,
                              const QModelIndex&) const override;
    bool eventFilter(QObject*, QEvent*) override;
};
