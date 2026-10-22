#include "ui/automatoneditor.h"

#include "domain/isomorphismchecker.h"
#include "ui/automatoncanvas.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QSplitter>
#include <QStyle>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
QString cellText(const QTableWidget* table, int row, int column) {
    const QTableWidgetItem* item = table->item(row, column);
    return item ? item->text().trimmed() : QString();
}

void ensureItems(QTableWidget* table) {
    for (int row = 0; row < table->rowCount(); ++row)
        for (int column = 0; column < table->columnCount(); ++column)
            if (!table->item(row, column)) table->setItem(row, column, new QTableWidgetItem);
}
}

AutomatonEditor::AutomatonEditor(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    m_tabs = new QTabWidget;
    m_tabs->addTab(createTablePage(), tr("Таблица"));
    m_tabs->addTab(createMatrixPage(), tr("Матрица"));
    m_tabs->addTab(createGraphPage(), tr("Граф"));
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    layout->addWidget(m_tabs);
}

QWidget* AutomatonEditor::createTablePage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    auto* description = new QGroupBox(tr("Описание автомата"));
    auto* descriptionLayout = new QVBoxLayout(description);
    auto* form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_typeLabel = new QLabel;
    m_statesEdit = new QLineEdit;
    m_inputsEdit = new QLineEdit;
    m_outputsEdit = new QLineEdit;
    m_initialCombo = new QComboBox;
    m_statesEdit->setPlaceholderText(tr("q0, q1, q2"));
    m_inputsEdit->setPlaceholderText(tr("x1, x2"));
    m_outputsEdit->setPlaceholderText(tr("y1, y2"));
    form->addRow(tr("Тип автомата:"), m_typeLabel);
    form->addRow(tr("Состояния:"), m_statesEdit);
    form->addRow(tr("Входные сигналы:"), m_inputsEdit);
    form->addRow(tr("Выходные сигналы:"), m_outputsEdit);
    form->addRow(tr("Начальное состояние:"), m_initialCombo);
    descriptionLayout->addLayout(form);
    auto* configure = new QPushButton(tr("Создать таблицу"));
    configure->setObjectName(QStringLiteral("configureTableButton"));
    configure->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    descriptionLayout->addWidget(configure, 0, Qt::AlignRight);
    layout->addWidget(description);
    m_transitionTable = new QTableWidget;
    m_transitionTable->setAlternatingRowColors(true);
    m_transitionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_transitionTable, 1);
    auto* hint = new QLabel(tr(
        "Мили: ячейка «следующее состояние / выход1, выход2». "
        "Мура: первая строка содержит выходы состояний, остальные — следующие состояния."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять таблицу"));
    accept->setObjectName(QStringLiteral("acceptTableButton"));
    accept->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_tableFeedback = new QLabel;
    m_tableFeedback->setWordWrap(true);
    layout->addWidget(accept, 0, Qt::AlignRight);
    layout->addWidget(m_tableFeedback);
    connect(configure, &QPushButton::clicked, this, &AutomatonEditor::configureTable);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptTable);
    connect(m_statesEdit, &QLineEdit::textEdited, this, &AutomatonEditor::invalidateFromTable);
    connect(m_inputsEdit, &QLineEdit::textEdited, this, &AutomatonEditor::invalidateFromTable);
    connect(m_outputsEdit, &QLineEdit::textEdited, this, &AutomatonEditor::invalidateFromTable);
    connect(m_initialCombo, &QComboBox::currentIndexChanged, this, [this] { invalidateFromTable(); });
    connect(m_transitionTable, &QTableWidget::itemChanged, this, [this] { invalidateFromTable(); });
    return page;
}

QWidget* AutomatonEditor::createMatrixPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    auto* row = new QHBoxLayout;
    m_matrixDimension = new QSpinBox;
    m_matrixDimension->setRange(1, 200);
    auto* create = new QPushButton(tr("Создать пустую матрицу"));
    create->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    row->addWidget(new QLabel(tr("Размерность:")));
    row->addWidget(m_matrixDimension);
    row->addWidget(create);
    row->addStretch();
    layout->addLayout(row);
    m_matrixTable = new QTableWidget;
    m_matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_matrixTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_matrixTable, 1);
    auto* hint = new QLabel(tr(
        "В каждой ячейке укажите метки дуг через «;». Для Мили: «вход / выход1,выход2»; "
        "для Мура: «вход». Заголовки Мура показывают выходы состояний из принятой таблицы."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять матрицу"));
    accept->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_matrixFeedback = new QLabel;
    m_matrixFeedback->setWordWrap(true);
    layout->addWidget(accept, 0, Qt::AlignRight);
    layout->addWidget(m_matrixFeedback);
    connect(create, &QPushButton::clicked, this, &AutomatonEditor::configureMatrix);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptMatrix);
    connect(m_matrixTable, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    return page;
}

QWidget* AutomatonEditor::createGraphPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    auto* splitter = new QSplitter(Qt::Vertical);
    auto* editor = new QWidget;
    auto* editorLayout = new QVBoxLayout(editor);
    m_edgeTable = new QTableWidget(0, 4);
    m_edgeTable->setHorizontalHeaderLabels(
        {tr("Из состояния"), tr("Вход"), tr("В состояние"), tr("Выходы (через запятую)")});
    m_edgeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(tr("Добавить дугу"));
    auto* remove = new QPushButton(tr("Удалить выбранную дугу"));
    add->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    remove->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    editorLayout->addLayout(buttons);
    editorLayout->addWidget(m_edgeTable);
    m_canvas = new AutomatonCanvas;
    splitter->addWidget(editor);
    splitter->addWidget(m_canvas);
    layout->addWidget(splitter, 1);
    auto* hint = new QLabel(tr(
        "Задайте дуги графа строками. Для автомата Мура оставьте столбец выходов пустым: "
        "выходы уже относятся к состояниям."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять граф"));
    accept->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_graphFeedback = new QLabel;
    m_graphFeedback->setWordWrap(true);
    layout->addWidget(accept, 0, Qt::AlignRight);
    layout->addWidget(m_graphFeedback);
    connect(add, &QPushButton::clicked, this, &AutomatonEditor::addGraphRow);
    connect(remove, &QPushButton::clicked, this, [this] {
        if (m_edgeTable->currentRow() >= 0)
            m_edgeTable->removeRow(m_edgeTable->currentRow());
    });
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptGraph);
    connect(m_edgeTable, &QTableWidget::itemChanged, this, [this] { invalidateGraph(); });
    return page;
}

void AutomatonEditor::setExpected(const Automaton& expected) {
    m_expected = expected;
    resetEditor();
    m_typeLabel->setText(automatonKindName(expected.kind));
    m_matrixDimension->setValue(expected.states.size());
}

void AutomatonEditor::resetEditor() {
    m_loading = true;
    m_statesEdit->clear();
    m_inputsEdit->clear();
    m_outputsEdit->clear();
    m_initialCombo->clear();
    m_transitionTable->clear();
    m_transitionTable->setRowCount(0);
    m_transitionTable->setColumnCount(0);
    m_matrixTable->clear();
    m_matrixTable->setRowCount(0);
    m_matrixTable->setColumnCount(0);
    m_edgeTable->setRowCount(0);
    m_canvas->clearAutomaton();
    m_tableFeedback->clear();
    m_matrixFeedback->clear();
    m_graphFeedback->clear();
    m_tableAccepted = m_matrixAccepted = m_graphAccepted = false;
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    m_tabs->setCurrentIndex(0);
    m_loading = false;
    emit completedChanged(false);
}

QStringList AutomatonEditor::parsedNames(const QString& text) const {
    QStringList values;
    for (const QString& part : text.split(QRegularExpression(QStringLiteral("[,;\\s]+")),
                                         Qt::SkipEmptyParts))
        values.append(part.trimmed());
    return values;
}

void AutomatonEditor::configureTable() {
    const QStringList states = parsedNames(m_statesEdit->text());
    const QStringList inputs = parsedNames(m_inputsEdit->text());
    const QStringList outputs = parsedNames(m_outputsEdit->text());
    if (states.isEmpty() || inputs.isEmpty() || outputs.isEmpty()) {
        showFeedback(m_tableFeedback, tr("Заполните списки состояний, входов и выходов."), false);
        return;
    }
    m_loading = true;
    m_initialCombo->clear();
    m_initialCombo->addItems(states);
    const bool moore = m_expected.kind == AutomatonKind::Moore;
    m_transitionTable->clear();
    m_transitionTable->setColumnCount(states.size());
    m_transitionTable->setRowCount(inputs.size() + (moore ? 1 : 0));
    m_transitionTable->setHorizontalHeaderLabels(states);
    QStringList vertical = inputs;
    if (moore) vertical.prepend(tr("Выход состояния"));
    m_transitionTable->setVerticalHeaderLabels(vertical);
    ensureItems(m_transitionTable);
    m_loading = false;
    invalidateFromTable();
    m_tableFeedback->setText(tr("Таблица создана. Заполните все существующие переходы."));
}

QSet<int> AutomatonEditor::parseOutputs(const QString& text,
                                        const Automaton& automaton, bool* ok) const {
    QSet<int> result;
    *ok = true;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("—") || trimmed == QStringLiteral("-"))
        return result;
    for (const QString& name : trimmed.split(QRegularExpression(QStringLiteral("[,\\s]+")),
                                             Qt::SkipEmptyParts)) {
        const int index = automaton.outputs.indexOf(name.trimmed());
        if (index < 0) { *ok = false; return {}; }
        result.insert(index);
    }
    return result;
}

QString AutomatonEditor::outputsText(const QSet<int>& outputs,
                                     const Automaton& automaton) const {
    QStringList result;
    for (int index : outputs) result.append(automaton.outputs.value(index));
    result.sort();
    return result.join(QLatin1Char(','));
}

bool AutomatonEditor::readTable(Automaton* result, QString* error) const {
    if (!result) return false;
    Automaton parsed;
    parsed.kind = m_expected.kind;
    parsed.states = parsedNames(m_statesEdit->text());
    parsed.inputs = parsedNames(m_inputsEdit->text());
    parsed.outputs = parsedNames(m_outputsEdit->text());
    parsed.initialState = m_initialCombo->currentIndex();
    if (m_transitionTable->columnCount() != parsed.states.size() ||
        m_transitionTable->rowCount() != parsed.inputs.size() +
            (parsed.kind == AutomatonKind::Moore ? 1 : 0)) {
        if (error) *error = tr("Пересоздайте таблицу после изменения списков имён.");
        return false;
    }
    int rowOffset = 0;
    if (parsed.kind == AutomatonKind::Moore) {
        parsed.mooreOutputs.resize(parsed.states.size());
        for (int state = 0; state < parsed.states.size(); ++state) {
            bool ok;
            parsed.mooreOutputs[state] = parseOutputs(cellText(m_transitionTable, 0, state), parsed, &ok);
            if (!ok) {
                if (error) *error = tr("Неизвестный выход в строке выходов состояний.");
                return false;
            }
        }
        rowOffset = 1;
    }
    for (int input = 0; input < parsed.inputs.size(); ++input) {
        for (int from = 0; from < parsed.states.size(); ++from) {
            const QString text = cellText(m_transitionTable, input + rowOffset, from);
            if (text.isEmpty() || text == QStringLiteral("—") || text == QStringLiteral("-")) continue;
            QString destination = text;
            QString outputPart;
            if (parsed.kind == AutomatonKind::Mealy) {
                const int slash = text.indexOf(QLatin1Char('/'));
                if (slash < 0) {
                    if (error) *error = tr("Для перехода Мили укажите «состояние / выход». ");
                    return false;
                }
                destination = text.left(slash).trimmed();
                outputPart = text.mid(slash + 1).trimmed();
            }
            const int to = parsed.states.indexOf(destination.trimmed());
            if (to < 0) {
                if (error) *error = tr("Переход ссылается на неизвестное состояние «%1».").arg(destination);
                return false;
            }
            Transition edge{from, input, to, {}};
            if (parsed.kind == AutomatonKind::Mealy) {
                bool ok;
                edge.outputs = parseOutputs(outputPart, parsed, &ok);
                if (!ok) {
                    if (error) *error = tr("Переход содержит неизвестный выход.");
                    return false;
                }
            }
            parsed.transitions.append(edge);
        }
    }
    if (!parsed.validate(error)) return false;
    *result = parsed;
    return true;
}

void AutomatonEditor::acceptTable() {
    Automaton actual;
    QString error;
    if (!readTable(&actual, &error)) {
        showFeedback(m_tableFeedback, error, false);
        return;
    }
    const IsomorphismResult check = IsomorphismChecker::compare(m_expected, actual);
    showFeedback(m_tableFeedback, check.explanation, check.equivalent);
    if (!check.equivalent) return;
    m_tableAutomaton = actual;
    m_tableAccepted = true;
    m_tabs->setTabEnabled(1, true);
    m_matrixDimension->setValue(actual.states.size());
    m_tabs->setCurrentIndex(1);
}

void AutomatonEditor::configureMatrix() {
    if (!m_tableAccepted) return;
    if (m_matrixDimension->value() != m_tableAutomaton.states.size()) {
        showFeedback(m_matrixFeedback,
                     tr("Размерность должна совпадать с числом состояний: %1.")
                         .arg(m_tableAutomaton.states.size()), false);
        return;
    }
    m_loading = true;
    const int size = m_tableAutomaton.states.size();
    m_matrixTable->clear();
    m_matrixTable->setRowCount(size);
    m_matrixTable->setColumnCount(size);
    QStringList headers;
    for (int state = 0; state < size; ++state) {
        QString header = m_tableAutomaton.states[state];
        if (m_tableAutomaton.kind == AutomatonKind::Moore)
            header += QStringLiteral(" / ") + outputsText(m_tableAutomaton.mooreOutputs[state],
                                                           m_tableAutomaton);
        headers.append(header);
    }
    m_matrixTable->setHorizontalHeaderLabels(headers);
    m_matrixTable->setVerticalHeaderLabels(m_tableAutomaton.states);
    ensureItems(m_matrixTable);
    m_loading = false;
    invalidateFromMatrix();
    m_matrixFeedback->setText(tr("Матрица создана. Заполните метки дуг."));
}

bool AutomatonEditor::readMatrix(Automaton* result, QString* error) const {
    if (!result || !m_tableAccepted) return false;
    Automaton parsed = m_tableAutomaton;
    parsed.transitions.clear();
    const int size = parsed.states.size();
    if (m_matrixTable->rowCount() != size || m_matrixTable->columnCount() != size) {
        if (error) *error = tr("Сначала создайте матрицу правильной размерности.");
        return false;
    }
    QSet<quint64> deterministicCells;
    for (int from = 0; from < size; ++from) {
        for (int to = 0; to < size; ++to) {
            const QString text = cellText(m_matrixTable, from, to);
            for (const QString& rawLabel : text.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
                QString label = rawLabel.trimmed();
                if (label.isEmpty() || label == QStringLiteral("—") || label == QStringLiteral("-")) continue;
                QString inputName = label;
                QString outputPart;
                if (parsed.kind == AutomatonKind::Mealy) {
                    const int slash = label.indexOf(QLatin1Char('/'));
                    if (slash < 0) {
                        if (error) *error = tr("Метка дуги Мили должна иметь вид «вход / выход».");
                        return false;
                    }
                    inputName = label.left(slash).trimmed();
                    outputPart = label.mid(slash + 1).trimmed();
                }
                const int input = parsed.inputs.indexOf(inputName.trimmed());
                if (input < 0) {
                    if (error) *error = tr("Матрица содержит неизвестный вход «%1».").arg(inputName);
                    return false;
                }
                const quint64 key = (quint64(quint32(from)) << 32) | quint32(input);
                if (deterministicCells.contains(key)) {
                    if (error) *error = tr("Для одной пары состояние-вход задано несколько дуг.");
                    return false;
                }
                deterministicCells.insert(key);
                Transition edge{from, input, to, {}};
                if (parsed.kind == AutomatonKind::Mealy) {
                    bool ok;
                    edge.outputs = parseOutputs(outputPart, parsed, &ok);
                    if (!ok) {
                        if (error) *error = tr("Матрица содержит неизвестный выход.");
                        return false;
                    }
                }
                parsed.transitions.append(edge);
            }
        }
    }
    if (!parsed.validate(error)) return false;
    *result = parsed;
    return true;
}

void AutomatonEditor::acceptMatrix() {
    Automaton actual;
    QString error;
    if (!readMatrix(&actual, &error)) {
        showFeedback(m_matrixFeedback, error, false);
        return;
    }
    const IsomorphismResult check = IsomorphismChecker::compare(m_expected, actual);
    showFeedback(m_matrixFeedback, check.explanation, check.equivalent);
    if (!check.equivalent) return;
    m_matrixAutomaton = actual;
    m_matrixAccepted = true;
    m_tabs->setTabEnabled(2, true);
    m_tabs->setCurrentIndex(2);
}

void AutomatonEditor::addGraphRow() {
    const int row = m_edgeTable->rowCount();
    m_edgeTable->insertRow(row);
    for (int column = 0; column < m_edgeTable->columnCount(); ++column)
        m_edgeTable->setItem(row, column, new QTableWidgetItem);
}

bool AutomatonEditor::readGraph(Automaton* result, QString* error) const {
    if (!result || !m_tableAccepted) return false;
    Automaton parsed = m_tableAutomaton;
    parsed.transitions.clear();
    for (int row = 0; row < m_edgeTable->rowCount(); ++row) {
        const QString fromName = cellText(m_edgeTable, row, 0);
        const QString inputName = cellText(m_edgeTable, row, 1);
        const QString toName = cellText(m_edgeTable, row, 2);
        const QString outputText = cellText(m_edgeTable, row, 3);
        if (fromName.isEmpty() && inputName.isEmpty() && toName.isEmpty() && outputText.isEmpty())
            continue;
        const int from = parsed.states.indexOf(fromName);
        const int input = parsed.inputs.indexOf(inputName);
        const int to = parsed.states.indexOf(toName);
        if (from < 0 || input < 0 || to < 0) {
            if (error) *error = tr("Строка %1 содержит неизвестное состояние или вход.").arg(row + 1);
            return false;
        }
        Transition edge{from, input, to, {}};
        if (parsed.kind == AutomatonKind::Mealy) {
            bool ok;
            edge.outputs = parseOutputs(outputText, parsed, &ok);
            if (!ok) {
                if (error) *error = tr("Строка %1 содержит неизвестный выход.").arg(row + 1);
                return false;
            }
        } else if (!outputText.isEmpty() && outputText != QStringLiteral("—")) {
            if (error) *error = tr("У автомата Мура выход задаётся для состояния, а не дуги.");
            return false;
        }
        parsed.transitions.append(edge);
    }
    if (!parsed.validate(error)) return false;
    *result = parsed;
    return true;
}

void AutomatonEditor::acceptGraph() {
    Automaton actual;
    QString error;
    if (!readGraph(&actual, &error)) {
        showFeedback(m_graphFeedback, error, false);
        return;
    }
    const IsomorphismResult check = IsomorphismChecker::compare(m_expected, actual);
    showFeedback(m_graphFeedback, check.explanation, check.equivalent);
    m_canvas->setAutomaton(actual);
    const bool wasComplete = m_graphAccepted;
    m_graphAccepted = check.equivalent;
    if (check.equivalent) m_graphAutomaton = actual;
    if (wasComplete != m_graphAccepted) emit completedChanged(m_graphAccepted);
}

void AutomatonEditor::invalidateFromTable() {
    if (m_loading) return;
    const bool wasComplete = m_graphAccepted;
    m_tableAccepted = m_matrixAccepted = m_graphAccepted = false;
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    if (wasComplete) emit completedChanged(false);
}

void AutomatonEditor::invalidateFromMatrix() {
    if (m_loading) return;
    const bool wasComplete = m_graphAccepted;
    m_matrixAccepted = m_graphAccepted = false;
    m_tabs->setTabEnabled(2, false);
    if (wasComplete) emit completedChanged(false);
}

void AutomatonEditor::invalidateGraph() {
    if (m_loading) return;
    const bool wasComplete = m_graphAccepted;
    m_graphAccepted = false;
    if (wasComplete) emit completedChanged(false);
}

void AutomatonEditor::showFeedback(QLabel* label, const QString& message, bool success) {
    label->setText((success ? QStringLiteral("✓ ") : QStringLiteral("⚠ ")) + message);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
}

QJsonArray AutomatonEditor::tableToJson(const QTableWidget* table) const {
    QJsonArray rows;
    for (int row = 0; row < table->rowCount(); ++row) {
        QJsonArray columns;
        for (int column = 0; column < table->columnCount(); ++column)
            columns.append(cellText(table, row, column));
        rows.append(columns);
    }
    return rows;
}

void AutomatonEditor::tableFromJson(QTableWidget* table, const QJsonArray& rows) {
    for (int row = 0; row < qMin(table->rowCount(), rows.size()); ++row) {
        const QJsonArray columns = rows[row].toArray();
        for (int column = 0; column < qMin(table->columnCount(), columns.size()); ++column)
            table->item(row, column)->setText(columns[column].toString());
    }
}

QJsonObject AutomatonEditor::toJson() const {
    QJsonObject object;
    object.insert(QStringLiteral("states"), m_statesEdit->text());
    object.insert(QStringLiteral("inputs"), m_inputsEdit->text());
    object.insert(QStringLiteral("outputs"), m_outputsEdit->text());
    object.insert(QStringLiteral("initial"), m_initialCombo->currentIndex());
    object.insert(QStringLiteral("table"), tableToJson(m_transitionTable));
    object.insert(QStringLiteral("matrixDimension"), m_matrixDimension->value());
    object.insert(QStringLiteral("matrix"), tableToJson(m_matrixTable));
    object.insert(QStringLiteral("edges"), tableToJson(m_edgeTable));
    object.insert(QStringLiteral("tableAccepted"), m_tableAccepted);
    object.insert(QStringLiteral("matrixAccepted"), m_matrixAccepted);
    object.insert(QStringLiteral("graphAccepted"), m_graphAccepted);
    return object;
}

void AutomatonEditor::fromJson(const QJsonObject& object) {
    m_loading = true;
    m_statesEdit->setText(object.value(QStringLiteral("states")).toString());
    m_inputsEdit->setText(object.value(QStringLiteral("inputs")).toString());
    m_outputsEdit->setText(object.value(QStringLiteral("outputs")).toString());
    m_loading = false;
    configureTable();
    m_loading = true;
    m_initialCombo->setCurrentIndex(object.value(QStringLiteral("initial")).toInt(0));
    tableFromJson(m_transitionTable, object.value(QStringLiteral("table")).toArray());
    m_loading = false;
    Automaton tableAutomaton;
    QString error;
    m_tableAccepted = object.value(QStringLiteral("tableAccepted")).toBool() &&
        readTable(&tableAutomaton, &error) &&
        IsomorphismChecker::compare(m_expected, tableAutomaton).equivalent;
    if (m_tableAccepted) {
        m_tableAutomaton = tableAutomaton;
        m_tabs->setTabEnabled(1, true);
        m_matrixDimension->setValue(object.value(QStringLiteral("matrixDimension")).toInt(
            m_tableAutomaton.states.size()));
        configureMatrix();
        m_loading = true;
        tableFromJson(m_matrixTable, object.value(QStringLiteral("matrix")).toArray());
        m_loading = false;
        Automaton matrixAutomaton;
        m_matrixAccepted = object.value(QStringLiteral("matrixAccepted")).toBool() &&
            readMatrix(&matrixAutomaton, &error) &&
            IsomorphismChecker::compare(m_expected, matrixAutomaton).equivalent;
        if (m_matrixAccepted) {
            m_matrixAutomaton = matrixAutomaton;
            m_tabs->setTabEnabled(2, true);
        }
    }
    const QJsonArray edges = object.value(QStringLiteral("edges")).toArray();
    m_loading = true;
    m_edgeTable->setRowCount(edges.size());
    ensureItems(m_edgeTable);
    tableFromJson(m_edgeTable, edges);
    m_loading = false;
    Automaton graphAutomaton;
    m_graphAccepted = m_matrixAccepted &&
        object.value(QStringLiteral("graphAccepted")).toBool() &&
        readGraph(&graphAutomaton, &error) &&
        IsomorphismChecker::compare(m_expected, graphAutomaton).equivalent;
    if (m_graphAccepted) {
        m_graphAutomaton = graphAutomaton;
        m_canvas->setAutomaton(graphAutomaton);
    }
    emit completedChanged(m_graphAccepted);
}
