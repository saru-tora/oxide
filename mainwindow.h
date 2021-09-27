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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "codeeditor.h"
#include "highlighter.h"
#include "nodemodel.h"
#include "welcome.h"
#include "wizard.h"

#include <QMainWindow>
#include <QUndoGroup>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSplitter>
#include <QHBoxLayout>
#include <QTemporaryDir>
#include <QTemporaryFile>

class QTextEdit;
class QAbstractItemModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

public slots:
    void about();
    void newProject();
    void open();
    void loadFile(const QString &path = QString());
    void saveFile();
    void saveAs();
    void build();
    void debug();
    void stopDebug();
    void runToLine();
    void stepInto();
    void nextLine();
    void stepOut();
    void breakPoint();
    void readDebugOutput();
    void closeTab(int index);
    void documentWasModified();

private:
    void setupEditor();
    void setupFileMenu();
    void setupHelpMenu();
    void setupDockWindows();
    void createActions();
    void changeTab(int index);

    enum class Db {started, interrupted, locals, none};
    QUndoGroup *undoGroup = nullptr;
    Db dbStatus = Db::none;
    bool welcomeVisible = true;
    int idNumber = 0;
    int exitCode = 0;
    int outFileSize = 0;
    QVector<QString> files;
    QVector<Client*> clients;
    Client *rls = nullptr;
    CodeEditor *currentEditor = nullptr;
    Welcome *welcome = nullptr;
    QTabWidget *tabWidget = nullptr;
    QTabWidget *management = nullptr;
    QTabWidget *logs = nullptr;
    QTreeView *treeView = nullptr;
    QTreeView *variableView = nullptr;
    QFileSystemModel *dirModel;
    NodeModel *nodeModel;
    QPlainTextEdit *issues = nullptr;
    QPlainTextEdit *applicationOutput = nullptr;
    QPlainTextEdit *compileOutput = nullptr;
    QProcess *process = nullptr;
    QProcess *db = nullptr;
    QTemporaryDir tempDir;
    QTemporaryFile tempFile;
    Wizard *wizard = nullptr;
};

#endif // MAINWINDOW_H
