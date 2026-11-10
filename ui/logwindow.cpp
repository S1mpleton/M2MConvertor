#include "ui/logwindow.h"
#include "infrastructure/logger.h"
#include "ui/appicons.h"
#include "ui/feedbackstyle.h"
#include "ui_logwindow.h"
#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QFileDialog>
#include <QFontDatabase>
#include <QSaveFile>
#include <QScrollBar>

LogWindow::LogWindow(QWidget* parent) : QDialog(parent), ui(new Ui::LogWindow) {
    ui->setupUi(this);
    setModal(false);
    setObjectName("eventLogWindow");
    ui->copyButton->setIcon(AppIcons::icon(AppIcons::Kind::Copy));
    ui->clearButton->setIcon(AppIcons::icon(AppIcons::Kind::Clear));
    ui->saveButton->setIcon(AppIcons::icon(AppIcons::Kind::Save));
    ui->logTextEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    connect(&Logger::instance(), &Logger::appended, this, [this](const Result& result) {
        if (Logger::instance().history().size() == 1000) {
            rebuild();
            return;
        }
        const int position = ui->logTextEdit->verticalScrollBar()->value();
        append(result);
        ui->logTextEdit->verticalScrollBar()->setValue(
            ui->autoScrollBox->isChecked() ? ui->logTextEdit->verticalScrollBar()->maximum()
                                           : position);
    });
    connect(&Logger::instance(), &Logger::cleared, this, &LogWindow::rebuild);
    for (auto* filter : {ui->showInfoBox, ui->showWarningsBox, ui->showErrorsBox})
        connect(filter, &QCheckBox::toggled, this, &LogWindow::rebuild);
    connect(ui->clearButton, &QPushButton::clicked, this, [] { Logger::instance().clear(); });
    connect(ui->saveButton, &QPushButton::clicked, this, &LogWindow::save);
    connect(ui->copyButton, &QPushButton::clicked, this, [this] {
        auto selection = ui->logTextEdit->textCursor().selectedText();
        QApplication::clipboard()->setText(selection.isEmpty()
                                               ? ui->logTextEdit->toPlainText()
                                               : selection.replace(QChar(0x2029), '\n'));
    });
    m_ready = true;
    rebuild();
}
LogWindow::~LogWindow() {
    delete ui;
}
void LogWindow::append(const Result& result) {
    if ((result.level() == ResultLevel::Info && !ui->showInfoBox->isChecked()) ||
        (result.level() == ResultLevel::Warning && !ui->showWarningsBox->isChecked()) ||
        (result.level() == ResultLevel::Error && !ui->showErrorsBox->isChecked()))
        return;
    QTextCharFormat format;
    format.setForeground(UiFeedbackStyle::foreground(
        result.level() == ResultLevel::Error     ? UiFeedbackLevel::Error
        : result.level() == ResultLevel::Warning ? UiFeedbackLevel::Warning
                                                 : UiFeedbackLevel::Success));
    auto cursor = ui->logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    if (!ui->logTextEdit->document()->isEmpty())
        cursor.insertBlock();
    cursor.insertText(Logger::line(result), format);
}
void LogWindow::rebuild() {
    const int position = ui->logTextEdit->verticalScrollBar()->value();
    ui->logTextEdit->clear();
    for (const auto& result : Logger::instance().history())
        append(result);
    ui->logTextEdit->verticalScrollBar()->setValue(
        ui->autoScrollBox->isChecked() ? ui->logTextEdit->verticalScrollBar()->maximum()
                                       : position);
}
Result LogWindow::saveTo(const QString& path) const {
    QSaveFile file(path);
    const auto bytes = ui->logTextEdit->toPlainText().toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return Result::error(
                   Message(QT_TRANSLATE_NOOP("Result", "Не удалось сохранить журнал: %1."))
                       .arg(path))
            .withDetails(file.errorString());
    return Result::success(Message(QT_TRANSLATE_NOOP("Result", "Журнал сохранён: %1.")).arg(path));
}
void LogWindow::save() {
    const auto path = QFileDialog::getSaveFileName(this, tr("Save Log"), "M2MConvertor.log.txt",
                                                   tr("Text files (*.txt);;All files (*)"));
    if (!path.isEmpty()) {
        const auto result = saveTo(path);
        Logger::instance().log(result);
        emit notification(result);
    }
}
void LogWindow::changeEvent(QEvent* event) {
    if (m_ready && event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        layout()->activate();
        rebuild();
    }
    if (m_ready && event->type() == QEvent::PaletteChange)
        rebuild();
    QDialog::changeEvent(event);
}
