/* Copyright (c) 2021, sarutora
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QtWidgets>

#include "highlighter.h"
#include "codeeditor.h"
#include "commands.h"

CodeEditor::CodeEditor(QWidget *parent, Client *client) : QPlainTextEdit(parent), rls(client)
{
    undoStack = new QUndoStack;
    lineNumberArea = new LineNumberArea(this);

    setMouseTracking(true);
    QString s = "[\\s]*//";
    commentExpression = QRegularExpression(s);
    endExpression = QRegularExpression("[{};,]\\s*$");
    blankExpression = QRegularExpression("^[\\s]*$");
    warningFormat.setFontUnderline(true);
    warningFormat.setUnderlineColor(Qt::yellow);
    warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setFontUnderline(true);
    errorFormat.setUnderlineColor(Qt::red);
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    connect(rls->ls, SIGNAL(readyReadStandardOutput()), this, SLOT(processResponse()));
    connect(rls->ls,static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &CodeEditor::rebootRls);
    rls->keep = true;

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));


    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
    verticalScrollBar()->setStyleSheet(QString::fromUtf8(
        "QScrollBar:vertical {"
        "    width:3px;    "
        "    margin: 0px 0px 0px 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: rgb(75, 75, 75);"
        "    min-height: 0px;"
        "}"
        "QScrollBar::add-line:vertical {"
        "    background: rgb(75, 75, 75);"
        "    height: 0px;"
        "    subcontrol-position: bottom;"
        "    subcontrol-origin: margin;"
        "}"
        "QScrollBar::sub-line:vertical {"
        "    background: rgb(75, 75, 75);"
        "    height: 0 px;"
        "    subcontrol-position: top;"
        "    subcontrol-origin: margin;"
        "}"
    ));
    horizontalScrollBar()->setStyleSheet(QString::fromUtf8(
        "QScrollBar:horizontal {"
        "    height:3px;    "
        "    margin: 0px 0px 0px 0px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: rgb(75, 75, 75);"
        "    min-width: 0px;"
        "}"
        "QScrollBar::add-line:horizontal {"
        "    background: rgb(75, 75, 75);"
        "    height: 0px;"
        "    subcontrol-position: right;"
        "    subcontrol-origin: margin;"
        "}"
        "QScrollBar::sub-line:horizontal {"
        "    background: rgb(75, 75, 75);"
        "    height: 0 px;"
        "    subcontrol-position: left;"
        "    subcontrol-origin: margin;"
        "}"
    ));
    c = new QCompleter(this);
    c->setModel(new QStringListModel(QStringList(), c));
    c->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    c->setWrapAround(false);
    setCompleter();
}

CodeEditor::~CodeEditor()
{
    if (rls) {
        QJsonObject textDocumentIdentifier;
        textDocumentIdentifier.insert("uri", uri);
        QJsonObject params;
        params.insert("textDocument", textDocumentIdentifier);
        QJsonObject closeNotification = rls->createRequest("textDocument/didClose", params);
        rls->sendRequest(closeNotification);
    }
}

void CodeEditor::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool CodeEditor::saveFile(const QString &fileName)
{
    QString errorMessage;

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    QSaveFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&file);
        out << toPlainText();
        if (!file.commit()) {
            errorMessage = tr("Cannot write file %1:\n%2.")
                           .arg(QDir::toNativeSeparators(fileName), file.errorString());
        }
    } else {
        errorMessage = tr("Cannot open file %1 for writing:\n%2.")
                       .arg(QDir::toNativeSeparators(fileName), file.errorString());
    }
    QGuiApplication::restoreOverrideCursor();

    if (!errorMessage.isEmpty()) {
        QMessageBox::warning(this, tr("Application"), errorMessage);
        return false;
    }

    setCurrentFile(fileName);
    return true;
}

void CodeEditor::setCurrentFile(const QString &fileName)
{
    filePath = fileName;
    document()->setModified(false);
    setWindowModified(false);

    QString shownName = filePath;
    if (filePath.isEmpty())
        shownName = "untitled.txt";
}

bool CodeEditor::save()
{
    if (filePath.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(filePath);
    }
}

bool CodeEditor::saveAs()
{
    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    return saveFile(dialog.selectedFiles().first());
}

void CodeEditor::open()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

void CodeEditor::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QTextStream in(&file);
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    setPlainText(in.readAll());
#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
}

bool CodeEditor::maybeSave()
{
    if (!document()->isModified())
        return true;
    const QMessageBox::StandardButton ret
        = QMessageBox::warning(this, tr("Application"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}

QJsonObject getPosition(QTextCursor tc) {
    QJsonObject position;
    int line = tc.blockNumber();
    int character = tc.positionInBlock();
    position.insert("line", line);
    position.insert("character", character);
    return position;
}

void CodeEditor::getCompletion() {
    QTextCursor tc = textCursor();
    QJsonObject params;
    QJsonObject textDocument;
    textDocument.insert("uri", uri);
    params.insert("textDocument", textDocument);
    params.insert("position", getPosition(tc));
    QJsonObject hoverRequest = rls->createRequest("textDocument/completion", params);
    rls->sendRequest(hoverRequest);
}

void CodeEditor::getTip(const QPoint &pos) {
    QTextCursor tc = cursorForPosition(pos);
    int bc = floor(log10(document()->blockCount()))+4;
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, bc);
    int p = tc.position();
    for (const auto &diagnostic: diagnostics) {
        if (p >= diagnostic.position && p < diagnostic.position + diagnostic.length) {
            codeTip = diagnostic.message;
            return;
        }
    }
    QJsonObject params;
    QJsonObject textDocument;
    textDocument.insert("uri", uri);
    params.insert("textDocument", textDocument);
    params.insert("position", getPosition(tc));
    QJsonObject hoverRequest = rls->createRequest("textDocument/hover", params);
    rls->sendRequest(hoverRequest);
}

bool CodeEditor::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        getTip(helpEvent->pos());
        if (!codeTip.isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), codeTip);
        } else {
            QToolTip::hideText();
            event->ignore();
        }
        return true;
    }
    return QPlainTextEdit::event(event);
}

void CodeEditor::matchBrackets()
{

    TextBlockData *data = static_cast<TextBlockData *>(textCursor().block().userData());

    if (data) {
        QVector<BracketInfo *> infos = data->brackets();

        int pos = textCursor().block().position();
        for (const auto pair: data->pairs()) {
            for (int i = 0; i < infos.size(); ++i) {
                BracketInfo *info = infos.at(i);

                int curPos = textCursor().position() - textCursor().block().position();
                QColor c;
                if (info->position == curPos - 1 && info->character == pair.first) {
                    if (matchLeftBracket(textCursor().block(), i + 1, 0, pair)) {
                        c = didMatch;
                    } else {
                        c = noMatch;
                    }
                    createBracketSelection(pos + info->position, c);
                } else if (info->position == curPos - 1 && info->character == pair.second) {
                    if (matchRightBracket(textCursor().block(), i - 1, 0, pair)) {
                        c = didMatch;
                    } else {
                        c = noMatch;
                    }
                    createBracketSelection(pos + info->position, c);
                }
            }
        }
        inString = false;
        for (int i = 0; i < infos.size(); ++i) {
            BracketInfo *info = infos.at(i);
            int curPos = textCursor().position() - textCursor().block().position();
            if (info->position < curPos) {
                if (info->character == '\'') {
                    inString = true;
                } else if (info->character == '\"') {
                    inString = false;
                }
            } else {
                break;
            }
        }
    }
}

int findBracketIndent(QTextBlock block) {
    while (block.isValid()) {
        QString text = block.text();
        TextBlockData *data = static_cast<TextBlockData *>(block.userData());
        QVector<BracketInfo *> infos = data->brackets();
        for (int i = 0; i < infos.size(); ++i) {
            BracketInfo *info = infos.at(i);
            if (info->character == '{') {
                int n = 0;
                static QString spaces = "    ";
                int start = block.text().indexOf(spaces);
                while (start != -1) {
                    n++;
                    start = block.text().indexOf(spaces, start + spaces.size());
                }
                return n;
            }
            if (info->character == '}') {
                return -1;
            }
        }
        block = block.previous();
    }
    return -1;
}

int findIndent(QTextBlock currentBlock) {
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<BracketInfo *> infos = data->brackets();

    int num = 0;
    for (int i = 0; i < infos.size(); ++i) {
        BracketInfo *info = infos.at(i);

        if (info->character == '{') {
            ++num;
            continue;
        }

        if (info->character == '}') {
            if (num > 0)
                --num;
        }
    }
    return num;
}

bool CodeEditor::matchLeftBracket(QTextBlock currentBlock, int i, int numLeftBrackets, QPair<char, char> pair)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<BracketInfo *> infos = data->brackets();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i) {
        BracketInfo *info = infos.at(i);

        if (info->character == pair.first) {
            ++numLeftBrackets;
            continue;
        }

        if (info->character == pair.second) {
            if (numLeftBrackets == 0) {
                createBracketSelection(docPos + info->position, QColor(42, 161, 179));
                return true;
            } else
                --numLeftBrackets;
        }
    }

    currentBlock = currentBlock.next();
    if (currentBlock.isValid())
        return matchLeftBracket(currentBlock, 0, numLeftBrackets, pair);

    return false;
}

bool CodeEditor::matchRightBracket(QTextBlock currentBlock, int i, int numRightBrackets, QPair<char, char> pair)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<BracketInfo *> brackets = data->brackets();

    int docPos = currentBlock.position();
    for (; i > -1 && brackets.size() > 0; --i) {
        BracketInfo *info = brackets.at(i);
        if (info->character == pair.second) {
            ++numRightBrackets;
            continue;
        }
        if (info->character == pair.first) {
            if (numRightBrackets == 0) {
                createBracketSelection(docPos + info->position, QColor(42, 161, 179));
                return true;
            } else
                --numRightBrackets;
        }
    }

    currentBlock = currentBlock.previous();
    if (currentBlock.isValid()) {
        data = static_cast<TextBlockData *>(currentBlock.userData());
        brackets = data->brackets();
        int sz = brackets.size();
        return matchRightBracket(currentBlock, sz-1, numRightBrackets, pair);
    }

    return false;
}

void CodeEditor::createBracketSelection(int pos, QColor color)
{
    QList<QTextEdit::ExtraSelection> selections = extraSelections();

    QTextEdit::ExtraSelection selection;
    QTextCharFormat format = selection.format;
    format.setBackground(color);
    selection.format = format;

    QTextCursor cursor = textCursor();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    selection.cursor = cursor;

    selections.append(selection);

    setExtraSelections(selections);
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits +2);

    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(51, 51, 51);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);

    matchBrackets();
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(51, 51, 51));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::lightGray);
            painter.drawText(-(2+fontMetrics().horizontalAdvance(QLatin1Char('9'))), top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
            if (breakpoints.contains(blockNumber)) {
                painter.setPen(Qt::red);
                painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                                 Qt::AlignLeft, "●");
            }
            if (stepNumber == blockNumber) {
                painter.setPen(Qt::yellow);
                painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                                 Qt::AlignLeft, "▶");
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::setCompleter()
{

    if(!c)
        return;

    c->setWidget(this);
        c->setCompletionMode(QCompleter::PopupCompletion);
        c->setCaseSensitivity(Qt::CaseInsensitive);
        QObject::connect(c, QOverload<const QString &>::of(&QCompleter::activated),
                         this, &CodeEditor::insertCompletion);
}

QCompleter *CodeEditor::completer() const
{
    return c;
}

void CodeEditor::insertCompletion(const QString &completion)
{
    if (c->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - c->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    setTextCursor(tc);
    pushAddCommand(completion.right(extra));
}

QString CodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void CodeEditor::focusInEvent(QFocusEvent *e)
{
    if (c)
        c->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

void CodeEditor::pushAddCommand(QString s)
{
    QString old = textCursor().selectedText();
    QUndoCommand *addCommand = new AddCommand(this, s, old);
    if (undoStack->index() < undoIndex)
        undoIndex = -1;
    undoStack->push(addCommand);
}

void CodeEditor::pushRemoveCommand(QString s)
{
    QUndoCommand *removeCommand = new RemoveCommand(this, s);
    undoStack->push(removeCommand);
}

int CodeEditor::getRange(QJsonObject range) {
    int line = range.value("line").toInt();
    int character = range.value("character").toInt();
    QTextBlock block = document()->findBlockByLineNumber(line);
    if (character > block.length())
        return -1;
    int pos = block.position();
    return pos + character;
}

void CodeEditor::sendChange(QTextCursor tc, QString text, int length, bool add) {
    QJsonObject params;
    QJsonObject versioned;
    version++;
    versioned.insert("uri", uri);
    versioned.insert("version", version);
    params.insert("textDocument", versioned);
    QJsonObject contentChangeEvent;
    QJsonArray contentChangeEvents;
    QJsonObject range;
    range.insert("end", getPosition(tc));
    tc.clearSelection();
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, length);
    range.insert("start", getPosition(tc));
    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, length);
    tc.removeSelectedText();
    if (add)
        tc.insertText(text);
    setTextCursor(tc);
    contentChangeEvent.insert("range", range);
    contentChangeEvent.insert("text", text);
    contentChangeEvents.append(contentChangeEvent);
    params.insert("contentChanges", contentChangeEvents);
    QJsonObject changeNotification = rls->createRequest("textDocument/didChange", params);
    if (!diagnostics.empty()) {
        tc.select(QTextCursor::Document);
        tc.setCharFormat(defFormat);
        diagnostics.clear();
    }
    rls->sendRequest(changeNotification);
}

void CodeEditor::rebootRls(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode != 0)
        std::cerr << exitCode << ' ' << exitStatus << "\n";

    if (rls->keep) {
        disconnect(rls->ls, SIGNAL(readyReadStandardOutput()), this, SLOT(processResponse()));
        disconnect(rls->ls,static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &CodeEditor::rebootRls);
        rls->start();
        connect(rls->ls, SIGNAL(readyReadStandardOutput()), this, SLOT(processResponse()));
        connect(rls->ls,static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &CodeEditor::rebootRls);
        QJsonObject params;
        QJsonObject versioned;
        version++;
        versioned.insert("uri", uri);
        versioned.insert("version", version);
        params.insert("textDocument", versioned);
        QJsonObject contentChangeEvent;
        QJsonArray contentChangeEvents;
        QFile file(filePath);

        QString raw = document()->toRawText();
        contentChangeEvent.insert("text", raw);
        contentChangeEvents.append(contentChangeEvent);
        params.insert("contentChanges", contentChangeEvents);
        QJsonObject changeNotification = rls->createRequest("textDocument/didChange", params);
        rls->sendRequest(changeNotification);
    }
}

void CodeEditor::processResponse()
{
    QVector<QJsonObject> responses = rls->getResponses();
    if (responses.isEmpty())
        return;
    QVector<Diagnostic> diags;
    for (const auto &response: responses) {
        if (response.value("method") == "textDocument/publishDiagnostics") {
            QJsonObject params = response.value("params").toObject();
            QJsonArray dv = params.value("diagnostics").toArray();
            std::cerr << dv.size() << '\n';
            for (const auto &diagnostic: dv) {
                QJsonObject d = diagnostic.toObject();
                QJsonObject r = d.value("range").toObject();
                int severity = d.value("severity").toInt();
                int start = getRange(r.value("start").toObject());
                int end = getRange(r.value("end").toObject());
                QString message = d.value("message").toString();
                std::cerr << start << ' ' << end << "\"" << message.toStdString() << "\"\n\n";
                if (start == -1)
                    continue;
                  diags.push_back({start, end-start, severity, message});
            }
        } else {
            QJsonValue res = response.value("result");
            if (res.isObject()) {
                QJsonObject result = res.toObject();
                QJsonObject contents = result.value("contents").toObject();
                codeTip = contents.value("value").toString();
            } else if (res.isArray()) {
                QJsonArray items = res.toArray();
                QStringList words;
                for (const auto &item: items) {
                    QString label = item.toObject().value("label").toString();
                    words << label;
                }
                QStringListModel* model = static_cast<QStringListModel*>(c->model());
                model->setStringList(words);
                maxRows = words.size();
                currentRow = 0;

                if (completionPrefix != c->completionPrefix()) {
                    c->setCompletionPrefix(completionPrefix);
                    c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
                }
                QRect cr = cursorRect();
                cr.setWidth(c->popup()->sizeHintForColumn(0)
                            + c->popup()->verticalScrollBar()->sizeHint().width());
                c->complete(cr);
            }
        }
    }
    for (const auto &d: diags) {
        QTextCursor tc = textCursor();
        tc.setPosition(d.position, QTextCursor::MoveAnchor);
        tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, d.length);
        if (d.severity == 1) {
            tc.setCharFormat(errorFormat);
        } else {
            tc.setCharFormat(warningFormat);
        }
    }
    diagnostics.append(diags);
}

void CodeEditor::braceIndent() {
    QTextCursor tc = textCursor();
    static QString spaces = "    ";
    QTextBlock block = tc.block();
    int n = findBracketIndent(block);
        if (blankExpression.match(block.text()).capturedStart() != -1) {
            tc.movePosition(QTextCursor::StartOfBlock);
            if (block.length() > 0) {
                tc.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                setTextCursor(tc);
            }
        }
    if (n != -1) {
        for (int i = 0; i < n; i++) {
            indent.append(spaces);
        }
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    indent.clear();
    QTextCursor tc0 = textCursor();
    tc0.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    QString selected = tc0.selectedText();
    QTextCursor tc = textCursor();
    bool b = false;
    bool ignore = false;
    QString spaces = "    ";

    if (!inString) {
        switch (e->key()) {
        case Qt::Key_BraceLeft:
            pushAddCommand("{}");
            b = true;
            break;
        case Qt::Key_BraceRight:
            {
                braceIndent();
                QString s = tc.selectedText();
                if (brackets > 0 && selected == "}")
                    ignore = true;
            }
            break;
        case Qt::Key_BracketLeft:
            pushAddCommand("[]");
            b = true;
            break;
        case Qt::Key_BracketRight:
            if (brackets > 0 && selected == "]")
                ignore = true;
            break;
        case Qt::Key_ParenLeft:
            pushAddCommand("()");
            b = true;
            break;
        case Qt::Key_ParenRight:
            if (brackets > 0 && selected == ")")
                ignore = true;
            break;
        default:
            break;
        }
    }
    int pos = tc.position();
    int anch = tc.anchor();
    auto leftSelect = [&]() {
        if (pos > anch) {
            tc.clearSelection();
            tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, pos - anch);
            tc.movePosition(QTextCursor::StartOfLine);
            int d = tc.position();
            tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, pos - d);
        } else {
            tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        }
    };
    auto reSelect = [&](QString &s) {
        pushAddCommand(s);
        if (pos > anch) {
            tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, s.size());
            tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, s.size());
        } else {
            tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, s.size());
        }
        setTextCursor(tc);
    };
    switch (e->key()) {
    case Qt::Key_Backspace:
        if (tc.hasSelection()) {
            pushRemoveCommand(tc.selectedText());
        } else {
                if (tc.position() > 0) {
                    tc.movePosition(QTextCursor::Left);
                    tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                    QString sel = tc.selectedText();
                    if (brackets > 0 && (sel == '(' || sel == '[' || sel == '{' || sel == '\'' || sel == '\"')) {
                        tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
                        setTextCursor(tc);
                        pushRemoveCommand(tc.selectedText());
                        return;
                    }
                }
                QString s = tc.selectedText();
                setTextCursor(tc);
                QUndoCommand *deleteCommand = new DeleteCommand(this, tc.selectedText());
                undoStack->push(deleteCommand);
        }
        return;
    case Qt::Key_X:
        if (e->modifiers().testFlag(Qt::ControlModifier)) {
            pushRemoveCommand(tc.selectedText());
            setTextCursor(tc);
        }
        break;
    case Qt::Key_V:
        if (e->modifiers().testFlag(Qt::ControlModifier)) {
            pushAddCommand(QGuiApplication::clipboard()->text());
            return;
        }
        break;
    case Qt::Key_Z:
        if (e->modifiers().testFlag(Qt::ControlModifier)) {
            if (e->modifiers().testFlag(Qt::ShiftModifier)) {
                undoStack->redo();
            } else {
                undoStack->undo();
            }
            return;
        }
        break;
    case Qt::Key_QuoteDbl:
        if (brackets > 0 && selected == "\"")
            ignore = true;
        else if (!inString) {
            pushAddCommand("\"\"");
            b = true;
        }
        break;
    case Qt::Key_Slash:
        if (e->modifiers().testFlag(Qt::ControlModifier)) {
            leftSelect();
            QString s = tc.selectedText();
            int n = 0;
            int start = 0;
            QVector<int> indexes{0};
            while (start != -1) {
                int end = s.indexOf(QChar::ParagraphSeparator, start);
                n++;
                if (end == -1)
                    break;
                start = end+1;
                indexes.push_back(start);
            }
            start = 0;
            int m = 0;
            for (int i = 0; i < n; i++) {
                int index = indexes.at(i);
                QRegularExpressionMatch match = commentExpression.match(s, start);
                int captured = match.capturedStart();
                if (captured != -1 && captured == index) {
                    m++;
                    start = s.indexOf(QChar::ParagraphSeparator, start)+1;
                } else {
                    break;
                }
            }
            if (n == m) {
                start = 0;
                while (start != -1) {
                    int captured = commentExpression.match(s).capturedEnd();
                    if (captured != -1)
                        s.remove(captured-2, 2);
                    start = captured;
                }
            } else {
                s.insert(0, "//");
                start = s.indexOf(QChar::ParagraphSeparator);
                while (start != -1) {
                    s.insert(start+1, "//");
                    start = s.indexOf(QChar::ParagraphSeparator, start+4);
                }
            }
            {
                setTextCursor(tc);
                pushAddCommand(s);
            }
            return;
        }
        break;
    case Qt::Key_Tab:
        {
            QString s = tc.selectedText();
            if (s.indexOf(QChar::ParagraphSeparator) == -1) {
                s = spaces;
                pushAddCommand(s);
                setTextCursor(tc);
                return;
            } else {
                leftSelect();
                s = tc.selectedText();
                int start = 0;
                s.insert(0, spaces);
                while (true) {
                    int end = s.indexOf(QChar::ParagraphSeparator, start);
                    if (end != -1) {
                        s.insert(end+1, spaces);
                        start = end + spaces.size();
                    }
                    else
                        break;
                }
                reSelect(s);
                return;
            }
        }
    case Qt::Key_Backtab:
    {
        leftSelect();
        QString s = tc.selectedText();
        int start = 0;
        while (true) {
            int end = s.indexOf(QChar::ParagraphSeparator, start);
            int idx = s.indexOf(spaces, start);
            if (idx != -1 && (idx < end || end == -1))
                s.remove(start, spaces.size());
            if (idx != -1 && end != -1) {
                start = end - spaces.size()+1;
            } else
                break;
        }
        reSelect(s);
        return;
    }
    case Qt::Key_Up:
        brackets = 0;
        break;
    case Qt::Key_Down:
        brackets = 0;
        break;
    case Qt::Key_Right:
        if (brackets > 0)
            movement++;
        if (movement > brackets) {
            brackets = 0;
            movement = 0;
        }
        break;
    case Qt::Key_Left:
        if (brackets > 0)
            movement--;
        if (movement < brackets) {
            brackets = 0;
            movement = 0;
        }
        break;
    case Qt::Key_Return:
        if (c && !c->popup()->isVisible()) {
            if (brackets > 0) {
                brackets = 0;
                movement = 0;
            }
            {
                int n = 0;
                if (selected == "}") {
                    n++;
                    indent.append("\n");
                } else if (selected == "\\") {
                    n++;
                    indent.append("\n");
                } else {
                    indent.append("\n");
                }
                QTextBlock blk = tc.block();
                while (blk.isValid()) {
                    if (commentExpression.match(blk.text()).capturedStart() == -1) {
                        int start = blk.text().indexOf(spaces);
                        while (start != -1) {
                            n++;
                            start = blk.text().indexOf(spaces, start + spaces.size());
                        }
                        n += findIndent(blk);
                        if (!blankExpression.match(blk.text()).hasMatch() && !endExpression.match(blk.text()).hasMatch())
                            n++;
                        break;
                    } else
                        blk = blk.previous();
                }
                for (int i = 0; i < n; i++) {
                    indent.append(spaces);
                }
                if (selected == "}") {
                    indent.append("\n");
                    n--;
                    for (int i = 0; i < n; i++) {
                        indent.append(spaces);
                    }
                }
                pushAddCommand(indent);
                if (selected == "}") {
                    tc.movePosition(QTextCursor::Left);
                    for (int i = 0; i < n; i++) {
                        tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, spaces.size());
                    }
                }
                setTextCursor(tc);
            }
            return;
        }
        else {
            e->ignore();
        }
        return;
    default:
        break;
    }
    if (b) {
        brackets++;
        tc.movePosition(QTextCursor::Left);
        setTextCursor(tc);
        return;
    } else if (ignore) {
        brackets--;
        tc.movePosition(QTextCursor::Right);
        setTextCursor(tc);
        return;
    }

    const bool ctrlOrShift = e->modifiers().testFlag(Qt::ControlModifier) ||
                             e->modifiers().testFlag(Qt::ShiftModifier);

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;

    if ((e->modifiers() == Qt::NoModifier || e->modifiers().testFlag(Qt::ShiftModifier)) && !e->text().isEmpty()) {
        indent.append(e->text());


        pushAddCommand(indent);
    }
    else {
        QPlainTextEdit::keyPressEvent(e);
    }

    completionPrefix = textUnderCursor();
    if (hasModifier || e->text().isEmpty()|| completionPrefix.length() < 3
                      || eow.contains(e->text().right(1))) {
        c->popup()->hide();
        return;
    }

    if (completionPrefix.length() >= 3 && !e->text().isEmpty()) {
        getCompletion();
    }

}
