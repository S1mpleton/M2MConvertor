#include "ui/uitext.h"
#include "ui/automatoneditor.h"

#include "domain/reactionchecker.h"
#include "ui/automatongrapheditor.h"
#include "ui/mealycelldelegate.h"
#include "ui/widgets/taskimage.h"
#include "ui/widgets/wrappinglabel.h"
#include "ui/widgets/hintsplitter.h"
#include "ui/plaincelldelegate.h"
#include "ui/widgets/workscrollarea.h"
#include "ui/widgets/actionbar.h"
#include <QSignalBlocker>
#include "ui/appicons.h"
#include "ui/workflowtabs.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QHelpEvent>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QPixmap>
#include <QStyle>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolTip>
#include <QTimer>
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
    UiText::tab(m_tabs, createTablePage(), QT_TRANSLATE_NOOP("App", "Таблица"));
    UiText::tab(m_tabs, createMatrixPage(), QT_TRANSLATE_NOOP("App", "Матрица"));
    UiText::tab(m_tabs, createGraphPage(), QT_TRANSLATE_NOOP("App", "Граф"));
    m_tabs->setTabEnabled(1, false);
    m_tabs->setTabEnabled(2, false);
    layout->addWidget(m_tabs);
    for (auto* table : {m_transitionTable, m_matrixTable, m_matrixOutputs,
                        m_matrixReference, m_graphReference}) {
        table->setShowGrid(false);
        table->viewport()->installEventFilter(this);
    }
    for (auto* reference : {m_matrixReference, m_graphReference})
        reference->setItemDelegate(new PlainCellDelegate(reference));
    m_ready = true;
    resizeMatrixHeaders();
}

QString AutomatonEditor::cellHelp(const QTableWidget* table, int row, int column) const {
    const auto inputs = tr("Один входной сигнал: x0, x1, …, x9. Например: x0. Перечисления не допускаются.");
    const auto outputs = tr("Выходные сигналы через запятую: y0 или y0, y2. «-» — нет выхода.");
    const auto states = tr("Состояния создаются автоматически: a0, a1, …; a0 — начальное. Эта ячейка не редактируется.");
    QString help;
    if (table == m_matrixTable) {
        help = tr("Переход из a%1 в a%2. ").arg(row).arg(column);
        help += m_editorKind == AutomatonKind::Moore
            ? tr("Входы на дугах: x0 или x0, x2. «-» — нет перехода.")
            : tr("Метки дуг: x0 / y0, y2; x1 / -. Разделяйте разные входы знаком «;». «-» — нет перехода.");
    } else if (table == m_matrixOutputs) {
        help = column == 0 ? states : outputs;
    } else {
        if (column == 0) help = inputs;
        else if (m_editorKind == AutomatonKind::Moore && row <= 1) help = outputs;
        else if (row < firstInputRow()) help = states;
        else if (m_editorKind == AutomatonKind::Moore)
            help = tr("Одно следующее состояние, например a0. Используйте только состояния из заголовка. «-» — нет перехода.");
        else
            help = tr("Следующее состояние / выходы: a0 / y0, y2 или a0 / -. Слева — одно состояние, справа — выходы через запятую. «-/-» — нет перехода.");
        if (table != m_transitionTable)
            help += "\n" + tr("Принятая таблица: только для просмотра.");
    }
    return help;
}

bool AutomatonEditor::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::ToolTip) {
        for (auto* table : {m_transitionTable, m_matrixTable, m_matrixOutputs,
                            m_matrixReference, m_graphReference}) {
            if (object != table->viewport()) continue;
            auto* help = static_cast<QHelpEvent*>(event);
            const auto index = table->indexAt(help->pos());
            if (index.isValid()) {
                const auto text = cellHelp(table, index.row(), index.column());
                QToolTip::showText(help->globalPos(), "<qt>" + text.toHtmlEscaped().replace('\n', "<br>") + "</qt>",
                                   table->viewport(), table->visualRect(index));
            } else {
                QToolTip::hideText();
                event->ignore();
            }
            return true;
        }
    }
    return QWidget::eventFilter(object, event);
}

QWidget* AutomatonEditor::createTablePage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(8);
    auto* work = new QWidget;
    auto* workLayout = new QVBoxLayout(work);
    workLayout->setContentsMargins(0, 0, 0, 0);
    workLayout->setSpacing(8);
    auto* setup = new ActionBar(Qt::AlignJustify);
    m_mealySetup = setup;
    m_clearTableButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Очистить таблицу"));
    m_clearTableButton->setObjectName("clearTableButton");
    m_clearTableButton->setIcon(AppIcons::icon(AppIcons::Kind::Clear));
    setup->addWidget(m_clearTableButton);
    auto* fields = new QWidget;
    auto* form = new QGridLayout(fields);
    form->setContentsMargins(0, 0, 0, 0);
    m_mealyStates = new QSpinBox;
    m_mealyInputs = new QSpinBox;
    m_mealyOutputs = new QSpinBox;
    m_mealyStates->setObjectName("mealyStateCount");
    m_mealyInputs->setObjectName("mealyInputCount");
    m_mealyOutputs->setObjectName("mealyOutputCount");
    m_mealyStates->setRange(0, 999999);
    m_mealyInputs->setRange(0, 10);
    m_mealyOutputs->setRange(0, 10);
    const QVector<QSpinBox*> counts{m_mealyStates, m_mealyInputs, m_mealyOutputs};
    const char* labels[] = {QT_TRANSLATE_NOOP("App", "Количество состояний:"),
        QT_TRANSLATE_NOOP("App", "Количество входных сигналов:"),
        QT_TRANSLATE_NOOP("App", "Количество выходных сигналов:")};
    for (int row = 0; row < counts.size(); ++row) {
        auto* label = UiText::make<QLabel>(labels[row]);
        label->setBuddy(counts[row]);
        UiText::bind(counts[row], "accessibleName", labels[row]);
        form->addWidget(label, row, 0);
        form->addWidget(counts[row], row, 1);
        connect(counts[row], &QSpinBox::valueChanged, this, [this] {
            if (m_loading) return;
            m_tableConfigured = false;
            m_acceptTableButton->setEnabled(false);
            invalidateFromTable();
        });
    }
    auto* build = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять"));
    build->setObjectName("configureTableButton");
    build->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    form->addWidget(build, 2, 2);
    setup->addWidget(fields, Qt::AlignRight);
    workLayout->addWidget(setup);
    m_tableContent = new QWidget;
    auto* layout = new QVBoxLayout(m_tableContent);
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
    UiText::bind(hint, "text", QT_TRANSLATE_NOOP("App", "Мили: ячейка «следующее состояние / выход1, выход2». "
        "Мура: выходы над состояниями. Введите входы x0…x9 слева; «-» означает отсутствие перехода или выхода."));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* accept = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять таблицу"));
    m_acceptTableButton = accept;
    accept->setObjectName(QStringLiteral("acceptTableButton"));
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptTable);
    connect(build, &QPushButton::clicked, this, &AutomatonEditor::configureMealyTable);
    connect(m_clearTableButton, &QPushButton::clicked, this, &AutomatonEditor::clearTable);
    connect(m_transitionTable, &QTableWidget::itemChanged, this, [this] { invalidateFromTable(); });
    auto* body = new QWidget;
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->addWidget(m_tableContent, 1);
    bodyLayout->addStretch();
    workLayout->addWidget(scrollWork(body), 1);
    auto* actions = new ActionBar;
    actions->addWidget(accept);
    workLayout->addWidget(actions);
    m_assignmentPanel = new HintSplitter(QT_TRANSLATE_NOOP("App", "Задание варианта"), m_assignmentImage, work, "tableAssignment");
    outer->addWidget(m_assignmentPanel, 1);
    return page;
}

QWidget* AutomatonEditor::createMatrixPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(8);
    auto* work = new QWidget;
    auto* layout = new QVBoxLayout(work);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    m_matrixReference = new QTableWidget;
    m_matrixReference->setObjectName("matrixReference");
    auto* row = new ActionBar(Qt::AlignJustify);
    m_clearMatrixButton = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Очистить матрицу"));
    m_clearMatrixButton->setObjectName("clearMatrixButton");
    m_clearMatrixButton->setIcon(AppIcons::icon(AppIcons::Kind::Clear));
    m_clearMatrixButton->setEnabled(false);
    row->addWidget(m_clearMatrixButton);
    auto* dimensions = new QWidget;
    auto* dimensionLayout = new QHBoxLayout(dimensions);
    dimensionLayout->setContentsMargins(0, 0, 0, 0);
    m_matrixDimension = new QSpinBox;
    m_matrixDimension->setRange(0, 999999);
    m_matrixDimension->setObjectName("matrixDimensionEdit");
    m_matrixDimension->setMaximumWidth(fontMetrics().horizontalAdvance("000000") + 28);
    UiText::bind(m_matrixDimension, "accessibleName", QT_TRANSLATE_NOOP("App", "Размерность матрицы"));
    auto* create = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять"));
    create->setObjectName("configureMatrixButton");
    create->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    dimensionLayout->addWidget(UiText::make<QLabel>(QT_TRANSLATE_NOOP("App", "Размерность:")));
    dimensionLayout->addWidget(m_matrixDimension);
    dimensionLayout->addWidget(create);
    row->addWidget(dimensions);
    layout->addWidget(row);
    m_matrixContent = new QWidget;
    auto* contentLayout = new QVBoxLayout(m_matrixContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);
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
    m_matrixOutputs->setHorizontalHeaderLabels({UiText::text(QT_TRANSLATE_NOOP("App", "Состояние")), UiText::text(QT_TRANSLATE_NOOP("App", "Выходные сигналы"))});
    m_matrixOutputs->verticalHeader()->hide();
    m_matrixOutputs->verticalHeader()->setDefaultSectionSize(36);
    m_matrixOutputs->setAlternatingRowColors(true);
    m_matrixOutputs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_matrixOutputs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_matrixOutputs->setItemDelegate(new PlainCellDelegate(m_matrixOutputs));
    m_outputCaption = new QLabel("Y =");
    tables->addWidget(m_outputCaption);
    tables->addWidget(m_matrixOutputs, 1);
    contentLayout->addLayout(tables, 1);
    auto* hint = new WrappingLabel;
    UiText::bind(hint, "text", QT_TRANSLATE_NOOP("App", "В каждой ячейке укажите метки дуг через «;». Для Мили: «вход / выход1,выход2»; "
        "для Мура: «x1, x2». Строка — откуда, столбец — куда. Выходы Мура заполните в таблице справа."));
    hint->setWordWrap(true);
    contentLayout->addWidget(hint);
    auto* accept = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять матрицу"));
    m_acceptMatrixButton = accept;
    accept->setEnabled(false);
    accept->setObjectName("acceptMatrixButton");
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    auto* body = new QWidget;
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->addWidget(m_matrixContent, 1);
    bodyLayout->addStretch();
    layout->addWidget(scrollWork(body), 1);
    m_matrixContent->hide();
    connect(m_clearMatrixButton, &QPushButton::clicked, this, &AutomatonEditor::clearMatrix);
    connect(create, &QPushButton::clicked, this, &AutomatonEditor::configureMatrix);
    connect(m_matrixDimension->findChild<QLineEdit*>(), &QLineEdit::returnPressed,
            this, &AutomatonEditor::configureMatrix);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptMatrix);
    connect(m_matrixTable, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    connect(m_matrixOutputs, &QTableWidget::itemChanged, this, [this] { invalidateFromMatrix(); });
    auto* actions = new ActionBar;
    actions->addWidget(accept);
    layout->addWidget(actions);
    m_matrixPanel = new HintSplitter(QT_TRANSLATE_NOOP("App", "Принятая таблица автомата"), m_matrixReference, work, "matrixHint");
    outer->addWidget(m_matrixPanel, 1);
    return page;
}

QWidget* AutomatonEditor::createGraphPage() {
    auto* page = new QWidget;
    auto* outer = new QVBoxLayout(page);
    outer->setContentsMargins(10, 10, 10, 10);
    outer->setSpacing(8);
    auto* work = new QWidget;
    auto* layout = new QVBoxLayout(work);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    m_graphReference = new QTableWidget;
    m_graphReference->setObjectName("graphReference");
    auto* buttons = new ActionBar(Qt::AlignLeft);
    auto* add = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Создать вершину"));
    add->setObjectName(QStringLiteral("createVertexButton"));
    auto* remove = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Удалить последнюю вершину"));
    remove->setObjectName(QStringLiteral("deleteVertexButton"));
    add->setIcon(AppIcons::icon(AppIcons::Kind::Add));
    remove->setIcon(AppIcons::icon(AppIcons::Kind::Remove));
    buttons->addWidget(add);
    buttons->addWidget(remove);
    layout->addWidget(buttons);
    auto* body = new QWidget;
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(8);
    m_graphEditor = new AutomatonGraphEditor;
    m_graphEditor->setObjectName(QStringLiteral("interactiveGraphEditor"));
    m_graphEditor->setMinimumHeight(120);
    remove->setEnabled(false);
    connect(m_graphEditor, &AutomatonGraphEditor::graphChanged, this, [this, add, remove] {
        const int count = m_graphEditor->vertices().size();
        add->setEnabled(count < 10);
        remove->setEnabled(count > 0);
    });
    bodyLayout->addWidget(m_graphEditor, 1);
    auto* hint = new WrappingLabel;
    hint->setObjectName("graphInstructions");
    UiText::bind(hint, "text", QT_TRANSLATE_NOOP("App", "Двойной щелчок изменяет выходы вершины Мура или подпись дуги. Выберите исходную вершину, "
        "затем удерживайте Ctrl и щёлкните по конечной вершине — появится дуга. "
        "Backspace/Delete удаляет последнюю вершину (выбранную дугу можно удалить отдельно)."));
    hint->setWordWrap(true);
    bodyLayout->addWidget(hint);
    layout->addWidget(scrollWork(body), 1);
    auto* accept = UiText::make<QPushButton>(QT_TRANSLATE_NOOP("App", "Принять граф"));
    accept->setObjectName("acceptGraphButton");
    accept->setIcon(AppIcons::icon(AppIcons::Kind::Accept));
    connect(add, &QPushButton::clicked, m_graphEditor,
            &AutomatonGraphEditor::createVertex);
    connect(remove, &QPushButton::clicked, m_graphEditor,
            &AutomatonGraphEditor::deleteLastVertex);
    connect(accept, &QPushButton::clicked, this, &AutomatonEditor::acceptGraph);
    connect(m_graphEditor, &AutomatonGraphEditor::graphChanged,
            this, &AutomatonEditor::invalidateGraph);
    auto* actions = new ActionBar;
    actions->addWidget(accept);
    layout->addWidget(actions);
    m_graphPanel = new HintSplitter(QT_TRANSLATE_NOOP("App", "Принятая таблица автомата"), m_graphReference, work, "graphHint");
    outer->addWidget(m_graphPanel, 1);
    return page;
}

void AutomatonEditor::setTask(const TrainingVariant& task, AutomatonKind kind) {
    m_task = task;
    m_editorKind = kind;
    m_mealySetup->setVisible(kind == AutomatonKind::Mealy);
    m_assignmentPanel->setExpanded(true); m_matrixPanel->setExpanded(true); m_graphPanel->setExpanded(true);
    resetEditor();
    m_matrixOutputs->setVisible(kind == AutomatonKind::Moore);
    m_outputCaption->setVisible(kind == AutomatonKind::Moore);
    m_assignmentImage->setImageData(task.diagram);
}

void AutomatonEditor::resetEditor() {
    m_loading = true;
    m_tableConfigured = m_editorKind == AutomatonKind::Moore;
    for (auto* count : {m_mealyStates, m_mealyInputs, m_mealyOutputs}) count->setValue(0);
    if (m_tableConfigured) configureTable();
    else {
        m_transitionTable->clearSpans();
        m_transitionTable->clear();
        m_transitionTable->setRowCount(0);
        m_transitionTable->setColumnCount(0);
        m_tableContent->hide();
    }
    m_acceptTableButton->setEnabled(m_tableConfigured);
    m_clearTableButton->setEnabled(m_tableConfigured);
    resetMatrix();
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
    m_tableContent->show();
    m_clearTableButton->setEnabled(true);
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
    m_transitionTable->item(0, 0)->setText(UiText::text(QT_TRANSLATE_NOOP("App", "Входные сигналы")));
    m_transitionTable->setSpan(0, 0, firstInputRow(), 1);
    if (moore) {
        m_transitionTable->item(0, 1)->setText(UiText::text(QT_TRANSLATE_NOOP("App", "Выходные сигналы")));
        if (counts.states > 1) m_transitionTable->setSpan(0, 1, 1, counts.states);
    }
    const int caption = firstInputRow() - 2;
    m_transitionTable->item(caption, 1)->setText(UiText::text(QT_TRANSLATE_NOOP("App", "Состояния")));
    if (counts.states > 1) m_transitionTable->setSpan(caption, 1, 1, counts.states);
    for (int state = 0; state < counts.states; ++state)
        m_transitionTable->item(firstInputRow() - 1, state + 1)->setText(QString("a%1").arg(state));
    m_transitionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_transitionTable->horizontalHeader()->setMinimumSectionSize(48);
    m_transitionTable->setColumnWidth(0, fontMetrics().horizontalAdvance(UiText::text(QT_TRANSLATE_NOOP("App", "Входные сигналы"))) + 32);
    m_loading = loading;
}

bool AutomatonEditor::mealyCountsMatch() const {
    const auto& counts = m_task.mealy;
    return m_mealyStates->value() == counts.states && m_mealyInputs->value() == counts.inputs &&
        m_mealyOutputs->value() == counts.outputs;
}

void AutomatonEditor::configureMealyTable() {
    if (m_editorKind != AutomatonKind::Mealy) return;
    for (auto* count : {m_mealyStates, m_mealyInputs, m_mealyOutputs}) count->interpretText();
    if (!mealyCountsMatch()) {
        showFeedback(Message(QT_TRANSLATE_NOOP("Result", "Количество состояний или сигналов автомата Мили не соответствует варианту.")), false, 0);
        return;
    }
    if (m_transitionTable->rowCount() == 0) configureTable();
    m_tableConfigured = true;
    m_acceptTableButton->setEnabled(true);
    showFeedback(Message(QT_TRANSLATE_NOOP("Result", "Размеры таблицы приняты.")), true, 0);
}

void AutomatonEditor::clearTable() {
    if (m_transitionTable->rowCount() == 0) return;
    configureTable();
    invalidateFromTable();
    showFeedback(Message(QT_TRANSLATE_NOOP("Result", "Таблица очищена.")), true, 0);
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

bool AutomatonEditor::readTable(Automaton* result, Message* error) const {
    if (!result || !m_tableConfigured) return false;
    Automaton parsed;
    parsed.kind = m_editorKind;
    parsed.states = ReactionChecker::stateNames(m_task.counts(m_editorKind).states);
    parsed.initialState = 0;
    QSet<QString> names;
    for (int row = firstInputRow(); row < m_transitionTable->rowCount(); ++row) {
        const auto input = cellText(m_transitionTable, row, 0);
        if (!QRegularExpression("^x[0-9]$").match(input).hasMatch() || names.contains(input)) {
            if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Заполните входные сигналы слева: разные x0…x9."));
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
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Выходы состояний: введите y0…y9 через запятую, без повторов, либо «-»."));
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
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Для перехода Мили укажите «состояние / выход». "));
                    return false;
                }
                destination = text.left(slash).trimmed();
                outputPart = text.mid(slash + 1).trimmed();
                if (destination == "-" || destination == QStringLiteral("—")) {
                    if (outputPart.isEmpty() || outputPart == "-" || outputPart == QStringLiteral("—")) continue;
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Без следующего состояния выходы не задаются. Для отсутствия перехода используйте -/-."));
                    return false;
                }
            }
            const int to = parsed.states.indexOf(destination.trimmed());
            if (to < 0) {
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Переход ссылается на неизвестное состояние «%1».")).arg(destination);
                return false;
            }
            Transition edge{from, input, to, {}};
            if (parsed.kind == AutomatonKind::Mealy) {
                bool ok;
                edge.outputs = parseOutputs(outputPart, parsed, &ok);
                if (!ok) {
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Выходы перехода: введите y0…y9 через запятую, без повторов, либо «-»."));
                    return false;
                }
            }
            parsed.transitions.append(edge);
        }
    }
    const auto valid = parsed.validation();
    if (!valid.ok()) { if (error) *error = valid.message(); return false; }
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
    Message error;
    if (!readTable(&actual, &error)) {
        showFeedback(error, false, 0);
        return;
    }
    const Result check = checkCandidate(actual);
    emit feedbackRequested(check.withContext(validationContext(0)));
    if (!check.ok()) return;
    const bool alreadyAccepted = m_tableAccepted;
    m_tableAutomaton = actual;
    m_tableAccepted = true;
    m_tabs->setTabEnabled(1, true);
    updateReference(m_matrixReference);
    updateReference(m_graphReference);
    if (!alreadyAccepted) resetMatrix();
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
    m_matrixDimension->interpretText();
    const int size = m_matrixDimension->value();
    if (size < 1) {
        showFeedback(Message(QT_TRANSLATE_NOOP("Result", "Введите размерность матрицы одним положительным целым числом.")), false);
        m_matrixDimension->setFocus();
        return;
    }
    if (size != m_tableAutomaton.states.size()) {
        showFeedback(Message(QT_TRANSLATE_NOOP("Result", "Размерность матрицы должна совпадать с числом состояний принятой таблицы: %1.")).arg(m_tableAutomaton.states.size()), false);
        m_matrixDimension->setFocus();
        return;
    }
    if (qint64(size) * size > 250000) {
        showFeedback(Message(QT_TRANSLATE_NOOP("Result", "Матрица слишком велика для редактора (более 250 000 ячеек).")), false);
        return;
    }
    if (m_matrixTable->rowCount() != size) buildMatrix(size);
}

void AutomatonEditor::resetMatrix() {
    const QSignalBlocker matrixBlocker(m_matrixTable), outputBlocker(m_matrixOutputs);
    m_matrixDimension->setValue(0);
    m_matrixTable->clear();
    m_matrixTable->setRowCount(0);
    m_matrixTable->setColumnCount(0);
    m_matrixOutputs->setRowCount(0);
    m_matrixContent->hide();
    m_clearMatrixButton->setEnabled(false);
    m_acceptMatrixButton->setEnabled(false);
    invalidateFromMatrix();
}

void AutomatonEditor::buildMatrix(int size) {
    const bool loading = m_loading;
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
    resizeMatrixHeaders();
    m_matrixContent->show();
    m_clearMatrixButton->setEnabled(true);
    m_acceptMatrixButton->setEnabled(true);
    m_loading = loading;
    invalidateFromMatrix();
}

void AutomatonEditor::clearMatrix() {
    if (m_matrixTable->rowCount() > 0) buildMatrix(m_matrixTable->rowCount());
}

void AutomatonEditor::resizeMatrixHeaders() {
    auto* outputsHeader = m_matrixOutputs->horizontalHeader();
    QFont headingFont = outputsHeader->font();
    headingFont.setBold(true); // Selected headers may use a bold font in the OS theme.
    const QFontMetrics headingMetrics(headingFont);
    const int padding = 2 * outputsHeader->style()->pixelMetric(QStyle::PM_HeaderMargin) + 14;
    const int stateWidth = headingMetrics.horizontalAdvance(m_matrixOutputs->horizontalHeaderItem(0)->text()) + padding;
    const int outputWidth = headingMetrics.horizontalAdvance(m_matrixOutputs->horizontalHeaderItem(1)->text()) + padding;
    m_matrixOutputs->setColumnWidth(0, stateWidth);
    // Reserve scrollbar space too, so adding rows cannot squeeze the output caption.
    m_matrixOutputs->setMinimumWidth(stateWidth + outputWidth + 2 * m_matrixOutputs->frameWidth() +
        style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    auto* matrixHeader = m_matrixTable->horizontalHeader();
    QFont matrixFont = matrixHeader->font(); matrixFont.setBold(true);
    const QString lastState = QString("a%1").arg(qMax(0, m_matrixTable->columnCount() - 1));
    const int axisWidth = qMax(48, QFontMetrics(matrixFont).horizontalAdvance(lastState) + padding);
    matrixHeader->setMinimumSectionSize(axisWidth);
    m_matrixTable->verticalHeader()->setMinimumWidth(axisWidth);
    m_matrixTable->setMinimumWidth(2 * axisWidth + 2 * m_matrixTable->frameWidth() +
        style()->pixelMetric(QStyle::PM_ScrollBarExtent));
}

void AutomatonEditor::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (m_ready && (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange))
        QTimer::singleShot(0, this, &AutomatonEditor::resizeMatrixHeaders);
}

bool AutomatonEditor::readMatrix(Automaton* result, Message* error) const {
    if (!result || !m_tableAccepted) return false;
    Automaton parsed = m_tableAutomaton;
    parsed.transitions.clear();
    parsed.outputs.clear();
    parsed.mooreOutputs.clear();
    const int size = parsed.states.size();
    if (m_matrixTable->rowCount() != size || m_matrixTable->columnCount() != size) {
        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Сначала создайте матрицу правильной размерности."));
        return false;
    }
    if (parsed.kind == AutomatonKind::Moore) {
        parsed.mooreOutputs.resize(size);
        for (int state = 0; state < size; ++state) {
            bool ok;
            parsed.mooreOutputs[state] = parseOutputs(cellText(m_matrixOutputs, state, 1), parsed, &ok);
            if (!ok) {
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Выходы состояния %1: нужны y0…y9 через запятую или «-»."))
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
                        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Метка дуги Мили должна иметь вид «вход / выход»."));
                        return false;
                    }
                    inputName = label.left(slash).trimmed();
                    outputPart = label.mid(slash + 1).trimmed();
                }
                const int input = parsed.inputs.indexOf(inputName.trimmed());
                if (input < 0) {
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Матрица содержит неизвестный вход «%1».")).arg(inputName);
                    return false;
                }
                const quint64 key = (quint64(quint32(from)) << 32) | quint32(input);
                if (deterministicCells.contains(key)) {
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Для одной пары состояние-вход задано несколько дуг."));
                    return false;
                }
                deterministicCells.insert(key);
                Transition edge{from, input, to, {}};
                if (parsed.kind == AutomatonKind::Mealy) {
                    bool ok;
                    edge.outputs = parseOutputs(outputPart, parsed, &ok);
                    if (!ok) {
                        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Выходы в матрице: нужны y0…y9 через запятую, без повторов, либо «-»."));
                        return false;
                    }
                }
                parsed.transitions.append(edge);
            }
        }
    }
    const auto valid = parsed.validation();
    if (!valid.ok()) { if (error) *error = valid.message(); return false; }
    *result = parsed;
    return true;
}

void AutomatonEditor::acceptMatrix() {
    m_matrixTable->setFocus();
    fillEmptyCells(m_matrixTable);
    fillEmptyCells(m_matrixOutputs);
    Automaton actual;
    Message error;
    if (!readMatrix(&actual, &error)) {
        showFeedback(error, false, 1);
        return;
    }
    const Result check = checkCandidate(actual);
    emit feedbackRequested(check.withContext(validationContext(1)));
    if (!check.ok()) return;
    m_matrixAutomaton = actual;
    m_matrixAccepted = true;
    Automaton graphTemplate = m_tableAutomaton;
    graphTemplate.mooreOutputs.clear();
    m_graphEditor->setTemplate(graphTemplate);
    m_tabs->setTabEnabled(2, true);
    m_tabs->setCurrentIndex(2);
}

bool AutomatonEditor::readGraph(Automaton* result, Message* error) const {
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
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Вершина автомата Мура должна иметь подпись «состояние / выходы»."));
                return false;
            }
            outputText = stateName.mid(slash + 1).trimmed();
            stateName = stateName.left(slash).trimmed();
        }
        const int state = parsed.states.indexOf(stateName);
        if (state < 0) {
            if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Вершина содержит неизвестное состояние «%1».")).arg(stateName);
            return false;
        }
        if (representedStates.contains(state)) {
            if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Состояние «%1» представлено несколькими вершинами.")).arg(stateName);
            return false;
        }
        representedStates.insert(state);
        stateByVertex.insert(vertex.id, state);
        if (parsed.kind == AutomatonKind::Moore) {
            bool ok;
            parsed.mooreOutputs[state] = parseOutputs(outputText, parsed, &ok);
            if (!ok) {
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Выходы вершины «%1»: нужны y0…y9 через запятую, без повторов, либо «-».")).arg(stateName);
                return false;
            }
        }
    }
    if (representedStates.size() != parsed.states.size()) {
        if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "На графе представлены не все состояния автомата."));
        return false;
    }

    for (const GraphEdgeData& graphEdge : m_graphEditor->edges()) {
        if (!stateByVertex.contains(graphEdge.fromId) ||
            !stateByVertex.contains(graphEdge.toId)) {
            if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Дуга ссылается на неизвестную вершину."));
            return false;
        }
        const QStringList labels = graphEdge.label.split(
            QRegularExpression(parsed.kind == AutomatonKind::Moore ? "[,;\\s]+" : "[;\\n]+"), Qt::SkipEmptyParts);
        if (labels.isEmpty()) {
            if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "У каждой дуги должна быть подпись."));
            return false;
        }
        for (const QString& rawLabel : labels) {
            QString inputName = rawLabel.trimmed();
            QString outputText;
            if (parsed.kind == AutomatonKind::Mealy) {
                const int slash = inputName.indexOf(QLatin1Char('/'));
                if (slash < 0) {
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Дуга Мили должна иметь подпись «вход / выходы»."));
                    return false;
                }
                outputText = inputName.mid(slash + 1).trimmed();
                inputName = inputName.left(slash).trimmed();
            } else if (inputName.contains(QLatin1Char('/'))) {
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "На дуге автомата Мура указывается только вход."));
                return false;
            }
            const int input = parsed.inputs.indexOf(inputName);
            if (input < 0) {
                if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Дуга содержит неизвестный вход «%1».")).arg(inputName);
                return false;
            }
            Transition edge{stateByVertex.value(graphEdge.fromId), input,
                            stateByVertex.value(graphEdge.toId), {}};
            if (parsed.kind == AutomatonKind::Mealy) {
                bool ok;
                edge.outputs = parseOutputs(outputText, parsed, &ok);
                if (!ok) {
                    if (error) *error = Message(QT_TRANSLATE_NOOP("Result", "Выходы дуги: нужны y0…y9 через запятую, без повторов, либо «-»."));
                    return false;
                }
            }
            parsed.transitions.append(edge);
        }
    }
    const auto valid = parsed.validation();
    if (!valid.ok()) { if (error) *error = valid.message(); return false; }
    *result = parsed;
    return true;
}

void AutomatonEditor::acceptGraph() {
    Automaton actual;
    Message error;
    if (!readGraph(&actual, &error)) {
        showFeedback(error, false, 2);
        return;
    }
    const Result check = checkCandidate(actual, true);
    emit feedbackRequested(check.withContext(validationContext(2)));
    const bool wasComplete = m_graphAccepted;
    m_graphAccepted = check.ok();
    if (check.ok()) m_graphAutomaton = actual;
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

Message AutomatonEditor::validationContext(int page) const {
    const auto kind = m_editorKind == AutomatonKind::Moore ? Message(QT_TRANSLATE_NOOP("Result", "Мура")) : Message(QT_TRANSLATE_NOOP("Result", "Мили"));
    const auto stage = page == 0 ? Message(QT_TRANSLATE_NOOP("Result", "Таблица")) : page == 1 ? Message(QT_TRANSLATE_NOOP("Result", "Матрица")) : Message(QT_TRANSLATE_NOOP("Result", "Граф"));
    return Message(QT_TRANSLATE_NOOP("Result", "%1 — %2")).arg(kind, stage);
}
void AutomatonEditor::showFeedback(const Message& message, bool success, int page) {
    emit feedbackRequested((success ? Result::success(message, ResultCategory::Validation) : Result::error(message, ResultCategory::Validation)).withContext(validationContext(page)));
}

Result AutomatonEditor::checkCandidate(const Automaton& candidate, bool graph) const {
    const auto result = ReactionChecker::check(m_task, candidate, graph);
    return result.result;
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
    object.insert("matrixDimension", m_matrixDimension->value());
    if (m_editorKind == AutomatonKind::Mealy)
        object.insert("mealySetup", QJsonObject{{"states", m_mealyStates->value()}, {"inputs", m_mealyInputs->value()},
            {"outputs", m_mealyOutputs->value()}, {"accepted", m_tableConfigured}});
    object.insert(QStringLiteral("graph"), m_graphEditor->toJson());
    object.insert(QStringLiteral("tableAccepted"), m_tableAccepted);
    object.insert(QStringLiteral("matrixAccepted"), m_matrixAccepted);
    object.insert(QStringLiteral("graphAccepted"), m_graphAccepted);
    return object;
}

QJsonObject AutomatonEditor::answerData() const {
    auto object = toJson();
    object.remove("hints");
    object.remove("hintExtents");
    // Generated headings may change with the UI language; only answers count as edits.
    const auto editableCells = [](const QTableWidget* table) {
        QJsonArray rows;
        for (int row = 0; row < table->rowCount(); ++row) {
            QJsonArray cells;
            for (int col = 0; col < table->columnCount(); ++col) {
                const auto* item = table->item(row, col);
                cells.append(item && (item->flags() & Qt::ItemIsEditable) ? item->text() : QString());
            }
            rows.append(cells);
        }
        return rows;
    };
    object.insert("table", editableCells(m_transitionTable));
    object.insert("matrixOutputs", editableCells(m_matrixOutputs));
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
    if (m_editorKind == AutomatonKind::Mealy) {
        const bool built = !object.value("table").toArray().isEmpty();
        const auto setup = object.value("mealySetup").toObject();
        const bool legacy = !object.contains("mealySetup");
        m_mealyStates->setValue(legacy && built ? m_task.mealy.states : setup.value("states").toInt());
        m_mealyInputs->setValue(legacy && built ? m_task.mealy.inputs : setup.value("inputs").toInt());
        m_mealyOutputs->setValue(legacy && built ? m_task.mealy.outputs : setup.value("outputs").toInt());
        if (built) configureTable();
        m_tableConfigured = built && mealyCountsMatch() && (legacy || setup.value("accepted").toBool());
        m_acceptTableButton->setEnabled(m_tableConfigured);
    }
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
    Message error;
    m_tableAccepted = object.value("tableAccepted").toBool() &&
        readTable(&actual, &error) && checkCandidate(actual).ok();
    if (m_tableAccepted) {
        m_tableAutomaton = actual;
        m_tabs->setTabEnabled(1, true);
        updateReference(m_matrixReference);
        updateReference(m_graphReference);
        const int size = m_tableAutomaton.states.size();
        const auto savedMatrix = object.value("matrix").toArray();
        if (savedMatrix.size() == size && qint64(size) * size <= 250000) {
            buildMatrix(size);
            m_matrixDimension->setValue(size);
        }
        if (object.contains("matrixDimension")) {
            const auto dimension = object.value("matrixDimension");
            m_matrixDimension->setValue(dimension.isString() ? dimension.toString().toInt() : dimension.toInt());
        }
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
            readMatrix(&actual, &error) && checkCandidate(actual).ok();
        if (m_matrixAccepted) {
            m_matrixAutomaton = actual;
            Automaton graphTemplate = m_tableAutomaton;
            graphTemplate.mooreOutputs.clear();
            m_graphEditor->setTemplate(graphTemplate);
            m_tabs->setTabEnabled(2, true);
            m_loading = true;
            QString graphError;
            const bool loaded = m_graphEditor->fromJson(object.value("graph").toObject(), &graphError);
            m_loading = false;
            m_graphAccepted = loaded && object.value("graphAccepted").toBool() &&
                readGraph(&actual, &error) && checkCandidate(actual, true).ok();
            if (m_graphAccepted) m_graphAutomaton = actual;
        }
    }
    emit completedChanged(m_graphAccepted);
}

void AutomatonEditor::retranslateTexts() {
    UiText::refresh(this);
    const bool loading = m_loading; m_loading = true;
    for (auto* table : {m_transitionTable, m_matrixReference, m_graphReference}) {
        if (table->rowCount() < firstInputRow() || table->columnCount() < 2) continue;
        table->item(0, 0)->setText(UiText::text(QT_TRANSLATE_NOOP("App", "Входные сигналы")));
        if (m_editorKind == AutomatonKind::Moore)
            table->item(0, 1)->setText(UiText::text(QT_TRANSLATE_NOOP("App", "Выходные сигналы")));
        table->item(firstInputRow() - 2, 1)->setText(UiText::text(QT_TRANSLATE_NOOP("App", "Состояния")));
        table->setColumnWidth(0, fontMetrics().horizontalAdvance(UiText::text(QT_TRANSLATE_NOOP("App", "Входные сигналы"))) + 32);
    }
    m_matrixOutputs->setHorizontalHeaderLabels({UiText::text(QT_TRANSLATE_NOOP("App", "Состояние")), UiText::text(QT_TRANSLATE_NOOP("App", "Выходные сигналы"))});
    resizeMatrixHeaders();
    m_loading = loading;
}
