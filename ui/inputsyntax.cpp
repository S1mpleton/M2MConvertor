#include "ui/inputsyntax.h"
#include <QLineEdit>
#include <QRegularExpression>
#include <QSet>

namespace {
QString list(QString text) {
    text.remove(QRegularExpression("^\\s+"));
    const bool trailing = !text.isEmpty() && (text.back().isSpace() || text.back() == ',');
    auto parts = text.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
    auto result = parts.join(", ");
    if (trailing && !result.isEmpty())
        result += ", ";
    return result;
}
} // namespace
InputSyntax::InputSyntax(Kind kind, QObject* parent) : QValidator(parent), m_kind(kind) {}
QString InputSyntax::normalize(QString text, Kind kind) {
    text.replace(QChar(0x2014), '-');
    if (kind == Outputs || kind == MooreEdges)
        return list(text);
    if (kind == MealyCell || kind == MealyEdges) {
        auto labels = text.split(';');
        for (auto& label : labels) {
            const int slash = label.indexOf('/');
            auto left = label.left(slash < 0 ? label.size() : slash);
            left.remove(QRegularExpression("\\s"));
            label = left + (slash < 0 ? QString() : "/" + list(label.mid(slash + 1)));
        }
        return labels.join(';');
    }
    text.remove(QRegularExpression("\\s"));
    return text;
}
QValidator::State InputSyntax::validate(QString& text, int& position) const {
    const auto prefix = normalize(text.left(position), m_kind);
    text = normalize(text, m_kind);
    position = qMin(prefix.size(), text.size());
    const QString output = "(?:-|y[0-9](?:, y[0-9])*)";
    QString pattern;
    switch (m_kind) {
    case Input:
        pattern = "x[0-9]";
        break;
    case StateName:
        pattern = "(?:-|a[0-9]+)";
        break;
    case Outputs:
        pattern = output;
        break;
    case MooreEdges:
        pattern = "(?:-|x[0-9](?:, x[0-9])*)";
        break;
    case MealyCell:
        pattern = "(?:-|a[0-9]+/" + output + ")";
        break;
    case MealyEdges:
        pattern = "(?:-|x[0-9]/" + output + "(?:;x[0-9]/" + output + ")*)";
        break;
    }
    if (text.isEmpty())
        return Intermediate;
    if (m_kind == Outputs || m_kind == MooreEdges || m_kind == MealyCell || m_kind == MealyEdges) {
        for (const auto& label : text.split(';')) {
            const auto values =
                label.mid(label.contains('/') ? label.indexOf('/') + 1 : 0).split(", ");
            QSet<QString> used;
            for (const auto& value : values) {
                if (value.size() != 2)
                    continue;
                if (used.contains(value))
                    return Invalid;
                used.insert(value);
            }
        }
    }
    const auto match = QRegularExpression("\\A(?:" + pattern + ")\\z")
                           .match(text, 0, QRegularExpression::PartialPreferCompleteMatch);
    return match.hasMatch() ? Acceptable : match.hasPartialMatch() ? Intermediate : Invalid;
}
void InputSyntax::fixup(QString& text) const {
    text = normalize(text, m_kind).trimmed();
    while (text.endsWith(','))
        text.chop(1);
    if (text.isEmpty() && m_kind != Input)
        text = "-";
}
SyntaxDelegate::SyntaxDelegate(Selector selector, QObject* parent)
    : QStyledItemDelegate(parent), m_selector(std::move(selector)) {}
QWidget* SyntaxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&,
                                      const QModelIndex& index) const {
    auto* edit = new QLineEdit(parent);
    edit->setValidator(new InputSyntax(m_selector(index), edit));
    return edit;
}
void SyntaxDelegate::setModelData(QWidget* widget, QAbstractItemModel* model,
                                  const QModelIndex& index) const {
    auto* edit = qobject_cast<QLineEdit*>(widget);
    if (!edit)
        return;
    QString value = edit->text();
    edit->validator()->fixup(value);
    int position = value.size();
    if (edit->validator()->validate(value, position) == QValidator::Acceptable)
        model->setData(index, value, Qt::EditRole);
}
