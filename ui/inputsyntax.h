#pragma once
#include <QStyledItemDelegate>
#include <QValidator>
#include <functional>

class InputSyntax : public QValidator {
  public:
    enum Kind { Input, StateName, Outputs, MealyCell, MooreEdges, MealyEdges };
    explicit InputSyntax(Kind kind, QObject* parent = nullptr);
    State validate(QString& text, int& position) const override;
    void fixup(QString& text) const override;
    static QString normalize(QString text, Kind kind);

  private:
    Kind m_kind;
};
class SyntaxDelegate : public QStyledItemDelegate {
  public:
    using Selector = std::function<InputSyntax::Kind(const QModelIndex&)>;
    SyntaxDelegate(Selector selector, QObject* parent);
    QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const override;
    void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const override;

  private:
    Selector m_selector;
};
