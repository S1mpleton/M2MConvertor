#include "ui/automatoneditor.h"

#include "domain/reactionchecker.h"
#include "ui/automatongrapheditor.h"
#include "ui/mealycelldelegate.h"
#include "ui/widgets/taskimage.h"
#include "ui/widgets/wrappinglabel.h"
#include "ui/widgets/hintsplitter.h"
#include "ui/plaincelldelegate.h"
#include "ui/widgets/workscrollarea.h"
#include <QSignalBlocker>
#include "ui/appicons.h"
#include "ui/workflowtabs.h"

#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
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
            if (!table->item(row, column)) table->setItem(row, column, new QTableWidgetItem("-"));
}
WorkScrollArea* scrollWork(QWidget* content) {
    return new WorkScrollArea(content);
}
}

AutomatonEditor::AutomatonEditor(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_tabs = new WorkflowTabs;
    m_tabs->addTab(createTablePage(), tr("Таблица"));
    m_tabs->addTab(createMatrixPage(), tr("Матрица"));
    m_tabs->addTab(createGraphPage(), tr("Граф"));
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    layout->addWidget(m_tabs);
}

QWidget* AutomatonEditor::createTablePage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(0);
    auto* work = new QWidget;
    auto* layout = new QVBoxLayout(work);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    m_assignmentImage = new TaskImage;
    m_assignmentImage->setObjectName("tableTaskImage");
    m_transitionTable = new QTableWidget;
    m_transitionTable->setObjectName("transitionTable");
    m_transitionTable->setAlternatingRowColors(true);
    m_transitionTable->horizontalHeader()->hide();
    m_transitionTable->verticalHeader()->hide();
    m_transitionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(m_transitionTable, 1);
    auto* hint = new WrappingLabel;
    hint->setText(tr(
        "Мили: ячейка «следующее состояние / выход1, выход2». "
        "Мура: выходы над состояниями. Введите входы x0…x9 слева; «-» означает отсутствие перехода или выхода."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять таблицу"));
    accept->setObjectName(QStringLiteral("acceptTableButton"));
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    layout->addWidget(accept, 0, Qt::AlignRight);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptTable);
    connect(m_transitionTable, &QTableWidget::itemChanged, this, [this] { invalidateFromTable(); });
    m_assignmentPanel = new HintSplitter(tr("Задание варианта"), m_assignmentImage, scrollWork(work), "tableAssignment");
    outer->addWidget(m_assignmentPanel);
    return page;
}

QWidget* AutomatonEditor::createMatrixPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(0);
    auto* work = new QWidget;
    auto* layout = new QVBoxLayout(work);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    m_matrixReference = new QTableWidget;
    m_matrixReference->setObjectName("matrixReference");
    auto* row = new QHBoxLayout;
    m_matrixDimension = new QLabel;
    auto* create = new QPushButton(tr("Очистить матрицу"));
    create->setObjectName("configureMatrixButton");
    create->setIcon(AppIcons::icon(AppIcons::Kind::Clear));
    row->addWidget(new QLabel(tr("Размерность:")));
    row->addWidget(m_matrixDimension);
    row->addWidget(create);
    row->addStretch();
    layout->addLayout(row);
    m_matrixTable = new QTableWidget;
    m_matrixTable->setObjectName("transitionMatrix");
    m_matrixTable->setItemDelegate(new PlainCellDelegate(m_matrixTable));
    m_matrixTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_matrixTable->verticalHeader()->setDefaultSectionSize(36);
    m_matrixTable->setAlternatingRowColors(true);
    auto* tables = new QHBoxLayout;
    tables->addWidget(new QLabel("C ="));
    tables->addWidget(m_matrixTable, 3);
    m_matrixOutputs = new QTableWidget;
    m_matrixOutputs->setObjectName("matrixOutputs");
    m_matrixOutputs->setColumnCount(2);
    m_matrixOutputs->setHorizontalHeaderLabels({tr("Состояние"), tr("Выходные сигналы")});
    m_matrixOutputs->verticalHeader()->hide();
    m_matrixOutputs->verticalHeader()->setDefaultSectionSize(36);
    m_matrixOutputs->setAlternatingRowColors(true);
    m_matrixOutputs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_matrixOutputs->setMinimumWidth(80);
    m_matrixOutputs->setItemDelegate(new PlainCellDelegate(m_matrixOutputs));
    m_outputCaption = new QLabel("Y =");
    tables->addWidget(m_outputCaption);
    tables->addWidget(m_matrixOutputs, 1);
    layout->addLayout(tables, 1);
    auto* hint = new WrappingLabel;
    hint->setText(tr(
        "В каждой ячейке укажите метки дуг через «;». Для Мили: «вход / выход1,выход2»; "
        "для Мура: «x1, x2». Строка — откуда, столбец — куда. Выходы Мура заполните в таблице справа."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять матрицу"));
    accept->setObjectName("acceptMatrixButton");
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    layout->addWidget(accept, 0, Qt::AlignRight);
    connect(create, &QPushButton::clicked, this, &AutomatonEditor::configureMatrix);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptMatrix);
    connect(m_matrixTable, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    connect(m_matrixOutputs, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    m_matrixPanel = new HintSplitter(tr("Принятая таблица автомата"), m_matrixReference, scrollWork(work), "matrixHint");
    outer->addWidget(m_matrixPanel);
    return page;
}

QWidget* AutomatonEditor::createGraphPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(0);
    auto* work = new QWidget;
    auto* layout = new QVBoxLayout(work);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    m_graphReference = new QTableWidget;
    m_graphReference->setObjectName("graphReference");
    auto* buttons = new QHBoxLayout;
    auto* add = new QPushButton(tr("Создать вершину"));
    add->setObjectName(QStringLiteral("createVertexButton"));
    auto* remove = new QPushButton(tr("Удалить последнюю вершину"));
    remove->setObjectName(QStringLiteral("deleteVertexButton"));
    add->setIcon(AppIcons::icon(AppIcons::Kind::Add));
    remove->setIcon(AppIcons::icon(AppIcons::Kind::Remove));
    buttons->addWidget(add);
    buttons->addWidget(remove);
    buttons->addStretch();
    layout->addLayout(buttons);
    m_graphEditor = new AutomatonGraphEditor;
    m_graphEditor->setObjectName(QStringLiteral("interactiveGraphEditor"));
    m_graphEditor->setMinimumHeight(120);
    remove->setEnabled(false);
    connect(m_graphEditor, &AutomatonGraphEditor::graphChanged, this, [this, add, remove] {
        const int count = m_graphEditor->vertices().size();
        add->setEnabled(count < 10);
        remove->setEnabled(count > 0);
    });
    layout->addWidget(m_graphEditor, 1);
    auto* hint = new WrappingLabel;
    hint->setObjectName("graphInstructions");
    hint->setText(tr(
        "Двойной щелчок изменяет выходы вершины Мура или подпись дуги. Можно создать до 10 состояний a0…a9. Выберите исходную вершину, "
        "затем удерживайте Ctrl и щёлкните по конечной вершине — появится дуга. "
        "Вершины можно перемещать, а дугу — перетаскивать для изменения изгиба. "
        "Backspace/Delete удаляет последнюю вершину (выбранную дугу можно удалить отдельно). Для Мили вершина подписывается «a0», "
        "дуга — «x1 / y1» или «x2 / -». Для Мура вершина — «a0 / y1», дуга — «x1»."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = new QPushButton(tr("Принять граф"));
    accept->setObjectName("acceptGraphButton");
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    layout->addWidget(accept, 0, Qt::AlignRight);
    connect(add, &QPushButton::clicked, m_graphEditor,
            &AutomatonGraphEditor::createVertex);
    connect(remove, &QPushButton::clicked, m_graphEditor,
            &AutomatonGraphEditor::deleteLastVertex);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptGraph);
    connect(m_graphEditor, &AutomatonGraphEditor::graphChanged,
            this, &AutomatonEditor::invalidateGraph);
    m_graphPanel = new HintSplitter(tr("Принятая таблица автомата"), m_graphReference, scrollWork(work), "graphHint");
    outer->addWidget(m_graphPanel);
    return page;
}

void AutomatonEditor::setTask(const TrainingVariant& task, AutomatonKind kind) {
    m_task = task;
    m_editorKind = kind;
    m_assignmentPanel->setExpanded(true); m_matrixPanel->setExpanded(true); m_graphPanel->setExpanded(true);
    resetEditor();
    const auto& counts = task.counts(kind);
    m_matrixDimension->setText(tr("%1 × %1").arg(counts.states));
    m_matrixOutputs->setVisible(kind == AutomatonKind::Moore);
    m_outputCaption->setVisible(kind == AutomatonKind::Moore);
    m_assignmentImage->setImageData(task.diagram);
}

void AutomatonEditor::resetEditor() {
    m_loading = true;
    configureTable();
    m_matrixTable->clear();
    m_matrixTable->setRowCount(0);
    m_matrixTable->setColumnCount(0);
    m_matrixOutputs->setRowCount(0);
    m_matrixReference->setRowCount(0);
    m_matrixReference->setColumnCount(0);
    m_graphReference->setRowCount(0);
    m_graphReference->setColumnCount(0);
    m_graphEditor->clearGraph();
    m_tableAccepted = m_matrixAccepted = m_graphAccepted = false;
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    m_tabs->setCurrentIndex(0);
    m_loading = false;
    emit completedChanged(false);
}

void AutomatonEditor::configureTable() {
    const auto& counts = m_task.counts(m_editorKind);
    const bool loading = m_loading;
    m_loading = true;
    m_transitionTable->clearSpans();
    m_transitionTable->clear();
    m_transitionTable->setColumnCount(counts.states + 1);
    m_transitionTable->setRowCount(firstInputRow() + counts.inputs);
    const bool moore = m_editorKind == AutomatonKind::Moore;
    // Reset the delegate when switching between Moore and Mealy variants.
    auto* oldDelegate = m_transitionTable->itemDelegate();
    m_transitionTable->setItemDelegate(moore ? new PlainCellDelegate(m_transitionTable)
                                            : new MealyCellDelegate(m_transitionTable));
    if (oldDelegate && oldDelegate->parent() == m_transitionTable) oldDelegate->deleteLater();
    for (int row = 0; row < m_transitionTable->rowCount(); ++row) {
        for (int col = 0; col < m_transitionTable->columnCount(); ++col) {
            const bool editable = row >= firstInputRow() || (moore && row == 1 && col > 0);
            auto* item = new QTableWidgetItem(editable ? (!moore && row >= firstInputRow() && col > 0 ? "-/-" : "-") : "");
            if (row >= firstInputRow() && col == 0) item->setText("");
            item->setTextAlignment(Qt::AlignCenter);
            if (!editable) {
                item->setFlags(Qt::ItemIsEnabled);
                QFont font = item->font(); font.setBold(true); item->setFont(font);
            }
            m_transitionTable->setItem(row, col, item);
        }
    }
    m_transitionTable->item(0, 0)->setText(tr("Входные сигналы"));
    m_transitionTable->setSpan(0, 0, firstInputRow(), 1);
    if (moore) {
        m_transitionTable->item(0, 1)->setText(tr("Выходные сигналы"));
        if (counts.states > 1) m_transitionTable->setSpan(0, 1, 1, counts.states);
    }
    const int caption = firstInputRow() - 2;
    m_transitionTable->item(caption, 1)->setText(tr("Состояния"));
    if (counts.states > 1) m_transitionTable->setSpan(caption, 1, 1, counts.states);
    for (int state = 0; state < counts.states; ++state)
        m_transitionTable->item(firstInputRow() - 1, state + 1)->setText(QString("a%1").arg(state));
    m_transitionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_transitionTable->horizontalHeader()->setMinimumSectionSize(48);
    m_transitionTable->setColumnWidth(0, fontMetrics().horizontalAdvance(tr("Входные сигналы")) + 32);
    m_loading = loading;
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
    parsed.initialState = 0;
    QSet<QString> names;
    for (int row = firstInputRow(); row < m_transitionTable->rowCount(); ++row) {
        const auto input = cellText(m_transitionTable, row, 0);
        if (!QRegularExpression("^x[0-9]$").match(input).hasMatch() || names.contains(input)) {
            if (error) *error = tr("Заполните входные сигналы слева: разные x0…x9.");
            return false;
        }
        names.insert(input); parsed.inputs.append(input);
    }
    const int rowOffset = firstInputRow();
    if (parsed.kind == AutomatonKind::Moore) {
        parsed.mooreOutputs.resize(parsed.states.size());
        for (int state = 0; state < parsed.states.size(); ++state) {
            bool ok;
            parsed.mooreOutputs[state] = parseOutputs(cellText(m_transitionTable, 1, state + 1), parsed, &ok);
            if (!ok) {
                if (error) *error = tr("Выходы состояний: введите y0…y9 через запятую, без повторов, либо «-».");
                return false;
            }
        }
    }
    for (int input = 0; input < parsed.inputs.size(); ++input) {
        for (int from = 0; from < parsed.states.size(); ++from) {
            const QString text = cellText(m_transitionTable, input + rowOffset, from + 1);
            if (text.isEmpty() || text == QStringLiteral("—") || text == QStringLiteral("-")) continue;
            QString destination = text;
            QString outputPart;
            if (parsed.kind == AutomatonKind::Mealy) {
                const int slash = text.indexOf(QLatin1Char('/'));
                if (slash < 0 || text.count('/') != 1) {
                    if (error) *error = tr("Для перехода Мили укажите «состояние / выход». ");
                    return false;
                }
                destination = text.left(slash).trimmed();
                outputPart = text.mid(slash + 1).trimmed();
                if (destination == "-" || destination == QStringLiteral("—")) {
                    if (outputPart.isEmpty() || outputPart == "-" || outputPart == QStringLiteral("—")) continue;
                    if (error) *error = tr("Без следующего состояния выходы не задаются. Для отсутствия перехода используйте -/-.");
                    return false;
                }
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

void AutomatonEditor::fillEmptyCells(QTableWidget* table) const {
    const QSignalBlocker blocker(table);
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            auto* item = table->item(row, col);
            if (!item || !(item->flags() & Qt::ItemIsEditable)) continue;
            auto text = item->text();
            if (table == m_transitionTable && m_editorKind == AutomatonKind::Mealy && col > 0 && row >= firstInputRow()) {
                const int slash = text.indexOf('/');
                if (text.trimmed().isEmpty()) text = "-/-";
                else if (slash >= 0 && text.count('/') == 1) {
                    const auto left = text.left(slash), right = text.mid(slash + 1);
                    text = (left.trimmed().isEmpty() ? "-" : left) + "/" +
                           (right.trimmed().isEmpty() ? "-" : right);
                }
            } else if (text.trimmed().isEmpty()) text = "-";
            if (text != item->text()) item->setText(text);
        }
    }
}

void AutomatonEditor::acceptTable() {
    m_transitionTable->setFocus();
    fillEmptyCells(m_transitionTable);
    Automaton actual;
    QString error;
    if (!readTable(&actual, &error)) {
        showFeedback(error, false);
        return;
    }
    const CandidateCheck check = checkCandidate(actual);
    showFeedback(check.explanation, check.accepted);
    if (!check.accepted) return;
    const bool alreadyAccepted = m_tableAccepted;
    m_tableAutomaton = actual;
    m_tableAccepted = true;
    m_tabs->setTabEnabled(1, true);
    if (!alreadyAccepted) configureMatrix();
    m_tabs->setCurrentIndex(1);
}

void AutomatonEditor::updateReference(QTableWidget* table) const {
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setFocusPolicy(Qt::NoFocus);
    table->setMinimumSize(0, 0);
    table->horizontalHeader()->hide(); table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setRowCount(m_transitionTable->rowCount());
    table->setColumnCount(m_transitionTable->columnCount());
    for (int row = 0; row < m_transitionTable->rowCount(); ++row) {
        for (int col = 0; col < m_transitionTable->columnCount(); ++col)
            table->setItem(row, col, m_transitionTable->item(row, col)->clone());
    }
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->setColumnWidth(0, m_transitionTable->columnWidth(0));
    table->clearSpans();
    for (int row = 0; row < firstInputRow(); ++row)
        for (int col = 0; col < m_transitionTable->columnCount(); ++col)
            if ((col == 0 && row == 0) || (col == 1 && row == firstInputRow() - 2) ||
                (m_editorKind == AutomatonKind::Moore && col == 1 && row == 0))
                if (m_transitionTable->rowSpan(row, col) > 1 || m_transitionTable->columnSpan(row, col) > 1)
                    table->setSpan(row, col, m_transitionTable->rowSpan(row, col),
                                               m_transitionTable->columnSpan(row, col));
}

void AutomatonEditor::configureMatrix() {
    if (!m_tableAccepted) return;
    updateReference(m_matrixReference);
    updateReference(m_graphReference);
    const int size = m_task.counts(m_editorKind).states;
    if (qint64(size) * size > 250000) {
        showFeedback(tr("Матрица слишком велика для редактора (более 250 000 ячеек)."), false);
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
        m_matrixOutputs->setItem(state, 1, new QTableWidgetItem("-"));
    }
    ensureItems(m_matrixTable);
    m_loading = false;
    invalidateFromMatrix();
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
            for (const QString& rawLabel : text.split(QRegularExpression(parsed.kind == AutomatonKind::Moore ? "[,;\\s]+" : ";"), Qt::SkipEmptyParts)) {
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
    m_matrixTable->setFocus();
    fillEmptyCells(m_matrixTable);
    fillEmptyCells(m_matrixOutputs);
    Automaton actual;
    QString error;
    if (!readMatrix(&actual, &error)) {
        showFeedback(error, false);
        return;
    }
    const CandidateCheck check = checkCandidate(actual);
    showFeedback(check.explanation, check.accepted);
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
    parsed.states = ReactionChecker::stateNames(m_graphEditor->vertices().size());
    parsed.initialState = 0;
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
            QRegularExpression(parsed.kind == AutomatonKind::Moore ? "[,;\\s]+" : "[;\\n]+"), Qt::SkipEmptyParts);
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
        showFeedback(error, false);
        return;
    }
    const CandidateCheck check = checkCandidate(actual, true);
    showFeedback(check.explanation, check.accepted);
    const bool wasComplete = m_graphAccepted;
    m_graphAccepted = check.accepted;
    if (check.accepted) m_graphAutomaton = actual;
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

void AutomatonEditor::showFeedback(const QString& message, bool success) {
    emit feedbackRequested(message, success);
}

AutomatonEditor::CandidateCheck
AutomatonEditor::checkCandidate(const Automaton& candidate, bool graph) const {
    const auto result = ReactionChecker::check(m_task, candidate, graph);
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
            if (table->item(row, column) && (table->item(row, column)->flags() & Qt::ItemIsEditable))
                table->item(row, column)->setText(columns[column].toString());
    }
}

QJsonObject AutomatonEditor::toJson() const {
    QJsonObject object;
    object.insert("tableLayout", 3);
    object.insert("hintExtents", QJsonObject{{"assignment", m_assignmentPanel->hintExtent()},
                                            {"matrix", m_matrixPanel->hintExtent()}, {"graph", m_graphPanel->hintExtent()}});
    object.insert("hints", QJsonObject{{"assignment", m_assignmentPanel->expanded()},
                                      {"matrix", m_matrixPanel->expanded()}, {"graph", m_graphPanel->expanded()}});
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
    const auto hints = object.value("hints").toObject();
    m_assignmentPanel->setExpanded(hints.value("assignment").toBool(true));
    m_matrixPanel->setExpanded(hints.value("matrix").toBool(true));
    m_graphPanel->setExpanded(hints.value("graph").toBool(true));
    const auto extents = object.value("hintExtents").toObject();
    if (extents.contains("assignment")) m_assignmentPanel->setHintExtent(qBound(0, extents.value("assignment").toInt(170), 10000));
    if (extents.contains("matrix")) m_matrixPanel->setHintExtent(qBound(0, extents.value("matrix").toInt(170), 10000));
    if (extents.contains("graph")) m_graphPanel->setHintExtent(qBound(0, extents.value("graph").toInt(170), 10000));
    m_loading = true;
    if (object.value("tableLayout").toInt() == 3) {
        tableFromJson(m_transitionTable, object.value("table").toArray());
    } else { // Migrate version 2 drafts; acceptance is rechecked with the new timing.
        const auto inputs = object.value("tableInputs").toArray();
        const auto rows = object.value("table").toArray();
        const bool moore = m_editorKind == AutomatonKind::Moore;
        for (int row = 0; row < inputs.size() && row < m_task.counts(m_editorKind).inputs; ++row)
            m_transitionTable->item(firstInputRow() + row, 0)->setText(inputs[row].toString());
        for (int row = 0; row < rows.size(); ++row) {
            const auto cells = rows[row].toArray();
            const int targetRow = moore && row == 0 ? 1 : firstInputRow() + row - (moore ? 1 : 0);
            if (targetRow >= m_transitionTable->rowCount()) break;
            for (int col = 0; col < cells.size() && col < m_task.counts(m_editorKind).states; ++col)
                m_transitionTable->item(targetRow, col + 1)->setText(cells[col].toString());
        }
    }
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
                readGraph(&actual, &error) && checkCandidate(actual, true).accepted;
            if (m_graphAccepted) m_graphAutomaton = actual;
        }
    }
    emit completedChanged(m_graphAccepted);
}
