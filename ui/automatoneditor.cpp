#include "ui/automatoneditor.h"

#include "domain/reactionchecker.h"
#include "ui/automatongrapheditor.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPixmap>
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
    m_criterionLabel = new QLabel;
    m_criterionLabel->setWordWrap(true);
    m_statesEdit = new QLineEdit;
    m_inputsEdit = new QLineEdit;
    m_statesEdit->setReadOnly(true);
    m_statesEdit->setObjectName("generatedStates");
    m_inputsEdit->setObjectName("inputSignals");
    m_inputsEdit->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\s*x[0-9](?:\\s*,\\s*x[0-9])*\\s*")), m_inputsEdit));
    m_inputsEdit->setPlaceholderText(tr("x1, x2"));
    form->addRow(tr("Тип автомата:"), m_typeLabel);
    form->addRow(tr("Критерий проверки:"), m_criterionLabel);
    form->addRow(tr("Состояния:"), m_statesEdit);
    form->addRow(tr("Входные сигналы:"), m_inputsEdit);
    form->addRow(tr("Начальное состояние:"), new QLabel(QStringLiteral("a0")));
    descriptionLayout->addLayout(form);
    auto* configure = new QPushButton(tr("Создать таблицу"));
    configure->setObjectName(QStringLiteral("configureTableButton"));
    configure->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    descriptionLayout->addWidget(configure, 0, Qt::AlignRight);
    m_assignmentImage = new QLabel;
    m_assignmentImage->setAlignment(Qt::AlignCenter);
    m_assignmentImage->setMaximumHeight(160);
    m_assignmentImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    layout->addWidget(m_assignmentImage);
    layout->addWidget(description);
    m_transitionTable = new QTableWidget;
    m_transitionTable->setObjectName("transitionTable");
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
    m_tableFeedback->setObjectName("tableFeedback");
    m_tableFeedback->setWordWrap(true);
    layout->addWidget(accept, 0, Qt::AlignRight);
    layout->addWidget(m_tableFeedback);
    connect(configure, &QPushButton::clicked, this, &AutomatonEditor::configureTable);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptTable);
    connect(m_inputsEdit, &QLineEdit::textChanged, this, &AutomatonEditor::invalidateFromTable);
    connect(m_transitionTable, &QTableWidget::itemChanged, this, [this] { invalidateFromTable(); });
    return page;
}

QWidget* AutomatonEditor::createMatrixPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    auto* referenceGroup = new QGroupBox(tr("Принятая таблица автомата"));
    auto* referenceLayout = new QVBoxLayout(referenceGroup);
    m_matrixReference = new QTableWidget;
    m_matrixReference->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_matrixReference->setFocusPolicy(Qt::NoFocus);
    m_matrixReference->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_matrixReference->setMaximumHeight(175);
    referenceLayout->addWidget(m_matrixReference);
    layout->addWidget(referenceGroup);
    auto* row = new QHBoxLayout;
    m_matrixDimension = new QLabel;
    auto* create = new QPushButton(tr("Очистить матрицу"));
    create->setObjectName("configureMatrixButton");
    create->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    row->addWidget(new QLabel(tr("Размерность:")));
    row->addWidget(m_matrixDimension);
    row->addWidget(create);
    row->addStretch();
    layout->addLayout(row);
    m_matrixTable = new QTableWidget;
    m_matrixTable->setObjectName("transitionMatrix");
    m_matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_matrixTable->verticalHeader()->setDefaultSectionSize(36);
    m_matrixTable->setAlternatingRowColors(true);
    auto* tables = new QHBoxLayout;
    tables->addWidget(m_matrixTable, 3);
    m_matrixOutputs = new QTableWidget;
    m_matrixOutputs->setObjectName("matrixOutputs");
    m_matrixOutputs->setColumnCount(2);
    m_matrixOutputs->setHorizontalHeaderLabels({tr("Состояние"), tr("Выходные сигналы")});
    m_matrixOutputs->verticalHeader()->hide();
    m_matrixOutputs->verticalHeader()->setDefaultSectionSize(36);
    m_matrixOutputs->setAlternatingRowColors(true);
    m_matrixOutputs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_matrixOutputs->setMinimumWidth(210);
    tables->addWidget(m_matrixOutputs, 1);
    layout->addLayout(tables, 1);
    auto* hint = new QLabel(tr(
        "В каждой ячейке укажите метки дуг через «;». Для Мили: «вход / выход1,выход2»; "
        "для Мура: «вход». Строка — откуда, столбец — куда. Выходы Мура заполните в таблице справа."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять матрицу"));
    accept->setObjectName("acceptMatrixButton");
    accept->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_matrixFeedback = new QLabel;
    m_matrixFeedback->setObjectName("matrixFeedback");
    m_matrixFeedback->setWordWrap(true);
    layout->addWidget(accept, 0, Qt::AlignRight);
    layout->addWidget(m_matrixFeedback);
    connect(create, &QPushButton::clicked, this, &AutomatonEditor::configureMatrix);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptMatrix);
    connect(m_matrixTable, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    connect(m_matrixOutputs, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    return page;
}

QWidget* AutomatonEditor::createGraphPage() {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(tr("Создать вершину"));
    add->setObjectName(QStringLiteral("createVertexButton"));
    auto* remove = new QPushButton(tr("Удалить выбранную вершину"));
    remove->setObjectName(QStringLiteral("deleteVertexButton"));
    add->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    remove->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    layout->addLayout(buttons);
    m_graphEditor = new AutomatonGraphEditor;
    m_graphEditor->setObjectName(QStringLiteral("interactiveGraphEditor"));
    layout->addWidget(m_graphEditor, 1);
    auto* hint = new QLabel(tr(
        "Двойной щелчок изменяет выходы вершины Мура или подпись дуги. Имена состояний заданы вариантом. Выберите исходную вершину, "
        "затем удерживайте Ctrl и щёлкните по конечной вершине — появится дуга. "
        "Вершины можно перемещать, а дугу — перетаскивать для изменения изгиба. "
        "Backspace/Delete удаляет выбранный элемент. Для Мили вершина подписывается «a0», "
        "дуга — «x1 / y1» или «x2 / -». Для Мура вершина — «a0 / y1», дуга — «x1»."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять граф"));
    accept->setObjectName("acceptGraphButton");
    accept->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_graphFeedback = new QLabel;
    m_graphFeedback->setObjectName("graphFeedback");
    m_graphFeedback->setWordWrap(true);
    layout->addWidget(accept, 0, Qt::AlignRight);
    layout->addWidget(m_graphFeedback);
    connect(add, &QPushButton::clicked, m_graphEditor,
            &AutomatonGraphEditor::createVertex);
    connect(remove, &QPushButton::clicked, m_graphEditor,
            &AutomatonGraphEditor::deleteSelectedVertex);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptGraph);
    connect(m_graphEditor, &AutomatonGraphEditor::graphChanged,
            this, &AutomatonEditor::invalidateGraph);
    return page;
}

void AutomatonEditor::setTask(const TrainingVariant& task, AutomatonKind kind) {
    m_task = task;
    m_editorKind = kind;
    resetEditor();
    m_typeLabel->setText(automatonKindName(kind));
    const auto& counts = task.counts(kind);
    m_criterionLabel->setText(tr(
        "Реакция варианта из a0; %1 состояний, %2 входных и %3 выходных сигналов. "
        "Мура: выход после перехода по входному сигналу.")
        .arg(counts.states).arg(counts.inputs).arg(counts.outputs));
    m_matrixDimension->setText(tr("%1 × %1").arg(counts.states));
    m_matrixOutputs->setVisible(kind == AutomatonKind::Moore);
    QPixmap diagram;
    diagram.loadFromData(task.diagram);
    m_assignmentImage->setPixmap(diagram.scaled(900, 160, Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation));
    m_assignmentImage->setVisible(!diagram.isNull());
}

void AutomatonEditor::resetEditor() {
    m_loading = true;
    m_statesEdit->setText(ReactionChecker::stateNames(m_task.counts(m_editorKind).states).join(", "));
    m_inputsEdit->clear();
    m_tableInputs.clear();
    m_transitionTable->clear();
    m_transitionTable->setRowCount(0);
    m_transitionTable->setColumnCount(0);
    m_matrixTable->clear();
    m_matrixTable->setRowCount(0);
    m_matrixTable->setColumnCount(0);
    m_matrixOutputs->setRowCount(0);
    m_matrixReference->setRowCount(0);
    m_matrixReference->setColumnCount(0);
    m_graphEditor->clearGraph();
    m_tableFeedback->clear();
    m_matrixFeedback->clear();
    m_graphFeedback->clear();
    m_tableAccepted = m_matrixAccepted = m_graphAccepted = false;
    m_tableFeedback->clear();
    m_matrixFeedback->clear();
    m_graphFeedback->clear();
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
    const QStringList states = ReactionChecker::stateNames(m_task.counts(m_editorKind).states);
    const QStringList inputs = parsedNames(m_inputsEdit->text());
    const QRegularExpression pattern(QStringLiteral("^x[0-9]$"));
    QSet<QString> unique;
    for (const auto& input : inputs) {
        if (!pattern.match(input).hasMatch() || unique.contains(input)) {
            showFeedback(m_tableFeedback, tr("Введите разные входы x0…x9 через запятую."), false);
            return;
        }
        unique.insert(input);
    }
    if (inputs.size() != m_task.counts(m_editorKind).inputs) {
        showFeedback(m_tableFeedback, tr("Требуется входных сигналов: %1.")
            .arg(m_task.counts(m_editorKind).inputs), false);
        return;
    }
    m_loading = true;
    m_tableInputs = inputs;
    const bool moore = m_editorKind == AutomatonKind::Moore;
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
                                        Automaton& automaton, bool* ok) const {
    QSet<int> result;
    *ok = true;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("—") || trimmed == QStringLiteral("-"))
        return result;
    const QRegularExpression pattern(QStringLiteral("^y[0-9]$"));
    for (const QString& raw : trimmed.split(QLatin1Char(','), Qt::KeepEmptyParts)) {
        const auto name = raw.trimmed();
        if (!pattern.match(name).hasMatch()) { *ok = false; return {}; }
        int index = automaton.outputs.indexOf(name);
        if (index < 0) {
            index = automaton.outputs.size();
            automaton.outputs.append(name);
        }
        if (result.contains(index)) { *ok = false; return {}; }
        result.insert(index);
    }
    return result;
}

bool AutomatonEditor::readTable(Automaton* result, QString* error) const {
    if (!result) return false;
    Automaton parsed;
    parsed.kind = m_editorKind;
    parsed.states = ReactionChecker::stateNames(m_task.counts(m_editorKind).states);
    parsed.inputs = parsedNames(m_inputsEdit->text());
    parsed.initialState = 0;
    if (parsed.inputs != m_tableInputs ||
        m_transitionTable->columnCount() != parsed.states.size() ||
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
                if (error) *error = tr("Выходы состояний: введите y0…y9 через запятую, без повторов, либо «-».");
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
                    if (error) *error = tr("Выходы перехода: введите y0…y9 через запятую, без повторов, либо «-».");
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
    const CandidateCheck check = checkCandidate(actual);
    showFeedback(m_tableFeedback, check.explanation, check.accepted);
    if (!check.accepted) return;
    const bool alreadyAccepted = m_tableAccepted;
    m_tableAutomaton = actual;
    m_tableAccepted = true;
    m_tabs->setTabEnabled(1, true);
    if (!alreadyAccepted) configureMatrix();
    m_tabs->setCurrentIndex(1);
}

void AutomatonEditor::configureMatrix() {
    if (!m_tableAccepted) return;
    m_matrixReference->setRowCount(m_transitionTable->rowCount());
    m_matrixReference->setColumnCount(m_transitionTable->columnCount());
    for (int row = 0; row < m_transitionTable->rowCount(); ++row) {
        m_matrixReference->setVerticalHeaderItem(row, m_transitionTable->verticalHeaderItem(row)->clone());
        for (int col = 0; col < m_transitionTable->columnCount(); ++col)
            m_matrixReference->setItem(row, col, m_transitionTable->item(row, col)->clone());
    }
    for (int col = 0; col < m_transitionTable->columnCount(); ++col)
        m_matrixReference->setHorizontalHeaderItem(col, m_transitionTable->horizontalHeaderItem(col)->clone());
    const int size = m_task.counts(m_editorKind).states;
    if (qint64(size) * size > 250000) {
        showFeedback(m_matrixFeedback, tr("Матрица слишком велика для редактора (более 250 000 ячеек)."), false);
        return;
    }
    m_loading = true;
    m_matrixTable->clear();
    m_matrixTable->setRowCount(size);
    m_matrixTable->setColumnCount(size);
    const auto states = ReactionChecker::stateNames(size);
    m_matrixTable->setHorizontalHeaderLabels(states);
    m_matrixTable->setVerticalHeaderLabels(states);
    m_matrixOutputs->setRowCount(m_editorKind == AutomatonKind::Moore ? size : 0);
    for (int state = 0; state < m_matrixOutputs->rowCount(); ++state) {
        auto* name = new QTableWidgetItem(states[state]);
        name->setFlags(name->flags() & ~Qt::ItemIsEditable);
        m_matrixOutputs->setItem(state, 0, name);
        m_matrixOutputs->setItem(state, 1, new QTableWidgetItem);
    }
    ensureItems(m_matrixTable);
    m_loading = false;
    invalidateFromMatrix();
    m_matrixFeedback->setText(tr("Матрица создана. Заполните метки дуг."));
}

bool AutomatonEditor::readMatrix(Automaton* result, QString* error) const {
    if (!result || !m_tableAccepted) return false;
    Automaton parsed = m_tableAutomaton;
    parsed.transitions.clear();
    parsed.outputs.clear();
    parsed.mooreOutputs.clear();
    const int size = parsed.states.size();
    if (m_matrixTable->rowCount() != size || m_matrixTable->columnCount() != size) {
        if (error) *error = tr("Сначала создайте матрицу правильной размерности.");
        return false;
    }
    if (parsed.kind == AutomatonKind::Moore) {
        parsed.mooreOutputs.resize(size);
        for (int state = 0; state < size; ++state) {
            bool ok;
            parsed.mooreOutputs[state] = parseOutputs(cellText(m_matrixOutputs, state, 1), parsed, &ok);
            if (!ok) {
                if (error) *error = tr("Выходы состояния %1: нужны y0…y9 через запятую или «-».")
                    .arg(parsed.states[state]);
                return false;
            }
        }
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
                        if (error) *error = tr("Выходы в матрице: нужны y0…y9 через запятую, без повторов, либо «-».");
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
    const CandidateCheck check = checkCandidate(actual);
    showFeedback(m_matrixFeedback, check.explanation, check.accepted);
    if (!check.accepted) return;
    m_matrixAutomaton = actual;
    m_matrixAccepted = true;
    Automaton graphTemplate = m_tableAutomaton;
    graphTemplate.mooreOutputs.clear();
    m_graphEditor->setTemplate(graphTemplate);
    m_tabs->setTabEnabled(2, true);
    m_tabs->setCurrentIndex(2);
}

bool AutomatonEditor::readGraph(Automaton* result, QString* error) const {
    if (!result || !m_tableAccepted) return false;
    Automaton parsed = m_tableAutomaton;
    parsed.transitions.clear();
    parsed.outputs.clear();
    if (parsed.kind == AutomatonKind::Moore)
        parsed.mooreOutputs = QVector<QSet<int>>(parsed.states.size());

    QHash<int, int> stateByVertex;
    QSet<int> representedStates;
    for (const GraphVertexData& vertex : m_graphEditor->vertices()) {
        QString stateName = vertex.label.trimmed();
        QString outputText;
        if (parsed.kind == AutomatonKind::Moore) {
            const int slash = stateName.indexOf(QLatin1Char('/'));
            if (slash < 0) {
                if (error) *error = tr(
                    "Вершина автомата Мура должна иметь подпись «состояние / выходы».");
                return false;
            }
            outputText = stateName.mid(slash + 1).trimmed();
            stateName = stateName.left(slash).trimmed();
        }
        const int state = parsed.states.indexOf(stateName);
        if (state < 0) {
            if (error) *error = tr("Вершина содержит неизвестное состояние «%1».").arg(stateName);
            return false;
        }
        if (representedStates.contains(state)) {
            if (error) *error = tr("Состояние «%1» представлено несколькими вершинами.").arg(stateName);
            return false;
        }
        representedStates.insert(state);
        stateByVertex.insert(vertex.id, state);
        if (parsed.kind == AutomatonKind::Moore) {
            bool ok;
            parsed.mooreOutputs[state] = parseOutputs(outputText, parsed, &ok);
            if (!ok) {
                if (error) *error = tr("Выходы вершины «%1»: нужны y0…y9 через запятую, без повторов, либо «-».").arg(stateName);
                return false;
            }
        }
    }
    if (representedStates.size() != parsed.states.size()) {
        if (error) *error = tr("На графе представлены не все состояния автомата.");
        return false;
    }

    for (const GraphEdgeData& graphEdge : m_graphEditor->edges()) {
        if (!stateByVertex.contains(graphEdge.fromId) ||
            !stateByVertex.contains(graphEdge.toId)) {
            if (error) *error = tr("Дуга ссылается на неизвестную вершину.");
            return false;
        }
        const QStringList labels = graphEdge.label.split(
            QRegularExpression(QStringLiteral("[;\\n]+")), Qt::SkipEmptyParts);
        if (labels.isEmpty()) {
            if (error) *error = tr("У каждой дуги должна быть подпись.");
            return false;
        }
        for (const QString& rawLabel : labels) {
            QString inputName = rawLabel.trimmed();
            QString outputText;
            if (parsed.kind == AutomatonKind::Mealy) {
                const int slash = inputName.indexOf(QLatin1Char('/'));
                if (slash < 0) {
                    if (error) *error = tr("Дуга Мили должна иметь подпись «вход / выходы».");
                    return false;
                }
                outputText = inputName.mid(slash + 1).trimmed();
                inputName = inputName.left(slash).trimmed();
            } else if (inputName.contains(QLatin1Char('/'))) {
                if (error) *error = tr("На дуге автомата Мура указывается только вход.");
                return false;
            }
            const int input = parsed.inputs.indexOf(inputName);
            if (input < 0) {
                if (error) *error = tr("Дуга содержит неизвестный вход «%1».").arg(inputName);
                return false;
            }
            Transition edge{stateByVertex.value(graphEdge.fromId), input,
                            stateByVertex.value(graphEdge.toId), {}};
            if (parsed.kind == AutomatonKind::Mealy) {
                bool ok;
                edge.outputs = parseOutputs(outputText, parsed, &ok);
                if (!ok) {
                    if (error) *error = tr("Выходы дуги: нужны y0…y9 через запятую, без повторов, либо «-».");
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

void AutomatonEditor::acceptGraph() {
    Automaton actual;
    QString error;
    if (!readGraph(&actual, &error)) {
        showFeedback(m_graphFeedback, error, false);
        return;
    }
    const CandidateCheck check = checkCandidate(actual);
    showFeedback(m_graphFeedback, check.explanation, check.accepted);
    const bool wasComplete = m_graphAccepted;
    m_graphAccepted = check.accepted;
    if (check.accepted) m_graphAutomaton = actual;
    if (wasComplete != m_graphAccepted) emit completedChanged(m_graphAccepted);
}

void AutomatonEditor::invalidateFromTable() {
    if (m_loading) return;
    const bool wasComplete = m_graphAccepted;
    m_tableAccepted = m_matrixAccepted = m_graphAccepted = false;
    m_tableFeedback->clear();
    m_matrixFeedback->clear();
    m_graphFeedback->clear();
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    if (wasComplete) emit completedChanged(false);
}

void AutomatonEditor::invalidateFromMatrix() {
    if (m_loading) return;
    const bool wasComplete = m_graphAccepted;
    m_matrixAccepted = m_graphAccepted = false;
    m_matrixFeedback->clear();
    m_graphFeedback->clear();
    m_tabs->setTabEnabled(2, false);
    if (wasComplete) emit completedChanged(false);
}

void AutomatonEditor::invalidateGraph() {
    if (m_loading) return;
    const bool wasComplete = m_graphAccepted;
    m_graphAccepted = false;
    m_graphFeedback->clear();
    if (wasComplete) emit completedChanged(false);
}

void AutomatonEditor::showFeedback(QLabel* label, const QString& message, bool success) {
    label->setText((success ? QStringLiteral("✓ ") : QStringLiteral("⚠ ")) + message);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
}

AutomatonEditor::CandidateCheck
AutomatonEditor::checkCandidate(const Automaton& candidate) const {
    const auto result = ReactionChecker::check(m_task, candidate);
    return {result.accepted, result.explanation};
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
    object.insert(QStringLiteral("inputs"), m_inputsEdit->text());
    object.insert(QStringLiteral("tableInputs"), QJsonArray::fromStringList(m_tableInputs));
    object.insert(QStringLiteral("table"), tableToJson(m_transitionTable));
    object.insert(QStringLiteral("matrixOutputs"), tableToJson(m_matrixOutputs));
    object.insert(QStringLiteral("matrix"), tableToJson(m_matrixTable));
    object.insert(QStringLiteral("graph"), m_graphEditor->toJson());
    object.insert(QStringLiteral("tableAccepted"), m_tableAccepted);
    object.insert(QStringLiteral("matrixAccepted"), m_matrixAccepted);
    object.insert(QStringLiteral("graphAccepted"), m_graphAccepted);
    return object;
}

void AutomatonEditor::fromJson(const QJsonObject& object) {
    resetEditor();
    const auto draftInputs = object.value("inputs").toString();
    QStringList tableInputs;
    for (const auto& input : object.value("tableInputs").toArray())
        tableInputs.append(input.toString());
    m_inputsEdit->setText(tableInputs.join(", "));
    if (!tableInputs.isEmpty()) configureTable();
    m_loading = true;
    tableFromJson(m_transitionTable, object.value("table").toArray());
    m_inputsEdit->setText(draftInputs);
    m_loading = false;
    Automaton actual;
    QString error;
    m_tableAccepted = object.value("tableAccepted").toBool() &&
        readTable(&actual, &error) && checkCandidate(actual).accepted;
    if (m_tableAccepted) {
        m_tableAutomaton = actual;
        m_tabs->setTabEnabled(1, true);
        configureMatrix();
        m_loading = true;
        tableFromJson(m_matrixTable, object.value("matrix").toArray());
        // State labels are generated, never trusted from a progress file.
        const auto rows = object.value("matrixOutputs").toArray();
        for (int row = 0; row < qMin(m_matrixOutputs->rowCount(), rows.size()); ++row) {
            const auto cells = rows[row].toArray();
            if (cells.size() > 1) m_matrixOutputs->item(row, 1)->setText(cells[1].toString());
        }
        m_loading = false;
        m_matrixAccepted = object.value("matrixAccepted").toBool() &&
            readMatrix(&actual, &error) && checkCandidate(actual).accepted;
        if (m_matrixAccepted) {
            m_matrixAutomaton = actual;
            Automaton graphTemplate = m_tableAutomaton;
            graphTemplate.mooreOutputs.clear();
            m_graphEditor->setTemplate(graphTemplate);
            m_tabs->setTabEnabled(2, true);
            m_loading = true;
            const bool loaded = m_graphEditor->fromJson(object.value("graph").toObject(), &error);
            m_loading = false;
            m_graphAccepted = loaded && object.value("graphAccepted").toBool() &&
                readGraph(&actual, &error) && checkCandidate(actual).accepted;
            if (m_graphAccepted) m_graphAutomaton = actual;
        }
    }
    emit completedChanged(m_graphAccepted);
}
