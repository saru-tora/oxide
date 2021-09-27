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

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include <QRegularExpression>
#include <QProcess>
#include <QUndoStack>
#include "lsp.h"

class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
class QCompleter;

class LineNumberArea;

class Diagnostic
{
public:
    Diagnostic(int position, int length, int severity, QString message): position(position), length(length), severity(severity), message(message) {}
    int position;
    int length;
    int severity;
    QString message;
};

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = 0, Client *client = nullptr);
    ~CodeEditor();

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void sendChange(QTextCursor tc, QString text, int length, bool add = true);
    void setCompleter();
    void loadFile(const QString &fileName);
    bool maybeSave();
    void setCurrentFile(const QString &fileName);
    QCompleter *completer() const;
    QUndoStack *undoStack;
    Client *rls = nullptr;
    QString uri;
    QString fileName;
    QString filePath;
    QVector<int> breakpoints;
    int stepNumber = -1;
    int undoIndex = 0;

protected:
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    bool saveAs();

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

    void matchBrackets();
    void insertCompletion(const QString &completion);
    void processResponse();
    void rebootRls(int exitCode, QProcess::ExitStatus exitStatus);
    void open();
    bool save();

private:
    QWidget *lineNumberArea;

    bool matchLeftBracket(QTextBlock currentBlock, int index, int numRightBrackets, QPair<char, char> pair);
    bool matchRightBracket(QTextBlock currentBlock, int index, int numLeftBrackets, QPair<char, char> pair);
    void createBracketSelection(int pos, QColor color);
    void pushAddCommand(QString s);
    void pushRemoveCommand(QString s);
    int getRange(QJsonObject range);
    void braceIndent();
    void getTip(const QPoint &pos);
    void getCompletion();
    bool saveFile(const QString &fileName);

    QString codeTip;
    QRegularExpression commentExpression;
    QRegularExpression endExpression;
    QRegularExpression blankExpression;

    QVector<Diagnostic> diagnostics;
    QTextCharFormat defFormat;
    QTextCharFormat warningFormat;
    QTextCharFormat errorFormat;
    QString textUnderCursor() const;
    QCompleter *c = nullptr;
    QColor didMatch = QColor(42, 161, 179);
    QColor noMatch = QColor(Qt::red);
    QString indent;
    QString completionPrefix;
    bool inString = false;
    int brackets = 0;
    int movement = 0;
    int version = 0;
    int currentRow = 0;
    int maxRows = 0;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};
#endif // CODEEDITOR_H
