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

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    process = new QProcess(this);
    db = new QProcess(this);
    undoGroup = new QUndoGroup;
    createActions();
    welcome = new Welcome;
    wizard = new Wizard;
    tabWidget = new QTabWidget(this);
    tabWidget->setTabsClosable(true);
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::changeTab);
    tabWidget->addTab(welcome, "Welcome");
    connect(welcome->newButton, SIGNAL(released()), this, SLOT(newProject()));
    connect(welcome->openButton, SIGNAL(released()), this, SLOT(open()));
    setupDockWindows();

    setCentralWidget(tabWidget);
    setStyleSheet("background-color: #232629; color: lightGray");
}

void MainWindow::changeTab(int index) {
    setWindowTitle(tabWidget->tabText(index) + " - " + tr("Oxide"));
    if (!welcomeVisible) {
        currentEditor = static_cast<CodeEditor*>(tabWidget->currentWidget());
        rls = currentEditor->rls;
    }
}

void MainWindow::closeTab(int index)
{
    if (!files.isEmpty()) {
        QString path = static_cast<CodeEditor*>(tabWidget->currentWidget())->filePath;
        int idx = files.indexOf(path);
        if (idx != -1) {
            files.remove(idx);
        }
        else
            throw "Error closing tab.";
    }
    else {
        setWindowTitle(tr("Oxide"));
        currentEditor = nullptr;
    }
    tabWidget->removeTab(index);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Oxide"),
                tr("<p><b>Oxide</b> is a simple IDE.</p><p>Copyright 2021 sarutora</p>"));
}

void MainWindow::newProject()
{
    wizard->show();
    if (!wizard->openName.isEmpty())
        loadFile(wizard->openName);
}


void MainWindow::runToLine()
{
    if (dbStatus == Db::interrupted) {
        QString cmd = "until " + QString::number(currentEditor->textCursor().blockNumber() + 1) + " \n";
        db->write(cmd.toStdString().c_str());
        dbStatus = Db::started;
    }
}

void MainWindow::stepInto()
{
    if (dbStatus == Db::interrupted) {
        db->write("step\n");
        dbStatus = Db::started;
    }
}

void MainWindow::nextLine()
{
    if (dbStatus == Db::interrupted) {
        db->write("next\n");
        dbStatus = Db::started;
    }
}

void MainWindow::stepOut()
{
    if (dbStatus == Db::interrupted) {
        db->write("finish\n");
        dbStatus = Db::started;
    }
}

QString getTime()
{
    QString time("[");
    time.append(QTime::currentTime().toString());
    time.append("] ");
    return time;
}

void MainWindow::stopDebug()
{
    if (dbStatus == Db::started) {
        db->write("\x03q\n");
    } else if (dbStatus == Db::interrupted) {
        db->write("q\n");
    }
    if (dbStatus != Db::none) {
        dbStatus = Db::none;
        currentEditor->stepNumber = -1;
        currentEditor->update();
        applicationOutput->appendPlainText(getTime() + "Debugging ended\n");
    }
}

void MainWindow::debug()
{
    if (exitCode != 0)
        return;
    if (dbStatus == Db::none) {
        QStringList args;
        QString dirName = rls->dirName;
        QString binName = dirName.right(dirName.size()-dirName.lastIndexOf('/'));
        args << "-quiet" << "-ex" << "set confirm off" << "-args" << dirName + "/target/debug" + binName;
        applicationOutput->appendPlainText(getTime() + "Debugging started");
        db->start("/usr/bin/gdb", args);
        for (int breakpoint: currentEditor->breakpoints) {
            QString bp = "break " + QString::number(breakpoint+1) + "\n";
            db->write(bp.toStdString().c_str());
        }
        outFileSize = 0;
        if (tempFile.open()) {
            std::string s = "run > " + tempFile.fileName().toStdString() + "\n";
            db->write(s.c_str());
        } else {
            throw "Error creating file for debugger";
        }
    } else if (dbStatus == Db::interrupted) {
        db->write("continue\n");
        dbStatus = Db::started;
    } else if (dbStatus == Db::started) {
        db->write("\x03");
        dbStatus = Db::interrupted;
    }
}

void MainWindow::build()
{
    QStringList args("build");
    process->start("/usr/bin/cargo", args);
    process->waitForFinished();
    exitCode = process->exitCode();
    QString s = process->readAllStandardError();
    if (exitCode == 0) {
        QString time = getTime() + '\n';
        time.append(s);
        compileOutput->appendPlainText(time);
    } else {
        issues->appendPlainText(s);
    }
}

void MainWindow::saveFile() {
    if (currentEditor) {
        QString text = currentEditor->document()->toPlainText();
        QString filePath = currentEditor->filePath;
        QFile file(filePath);
        if (file.open(QFile::ReadWrite | QFile::Text)) {
            QTextStream stream(&file);
            stream << text;
                currentEditor->undoIndex = currentEditor->undoStack->index();
            documentWasModified();
        }
    }
}

void MainWindow::loadFile(const QString &path)
{
    QString fileName = path;

    if (fileName.isNull())
        fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", "Rust Files (*.rs *.toml)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        QString ext = fileName.right(fileName.size() - fileName.lastIndexOf('.') - 1);
        if (ext == "rs" && file.open(QFile::ReadOnly | QFile::Text)) {
            QDir dir(path.left(path.lastIndexOf('/')));
            dir.cdUp();
            QString dirName;
            while (!dir.isRoot()) {
                if (dir.exists("Cargo.toml")) {
                    dirName = dir.path();
                    break;
                }
                dir.cdUp();
            }
            QString wsname = dirName.right(path.size() - dirName.lastIndexOf('/')-1);

            bool b = true;
            for (const auto client: clients) {
                if (client->dirName == dirName) {
                    b = false;
                    break;
                }
            }

            if (!dirName.isEmpty() && b) {
                if (clients.empty() && tabWidget->count() == 1) {
                    tabWidget->removeTab(0);
                    welcomeVisible = false;
                }
                Client *client = new Client(parent(), QCoreApplication::applicationPid(), dirName);
                clients.append(client);
                rls = client;
                QDir::setCurrent(dirName);
                QString name = tempDir.path() + dirName.right(dirName.size() - dirName.lastIndexOf('/'));
                QFile::link(dirName, name);
            }
            setupEditor();
            files.append(fileName);
            QString name = path.right(path.size() - path.lastIndexOf('/')-1);
            tabWidget->addTab(currentEditor, name);
            QString text = file.readAll();
            currentEditor->setPlainText(text);
            QJsonObject params;
            QJsonObject textDocument;
            QString uri = "file://" + path;
            currentEditor->uri = uri;
            currentEditor->filePath = path;
            currentEditor->fileName = name;
            textDocument.insert("uri", uri);
            textDocument.insert("languageId", "rust");
            textDocument.insert("version", 0);
            textDocument.insert("text", text);
            params.insert("textDocument", textDocument);
            QJsonObject didOpen = rls->createRequest("textDocument/didOpen", params);
            rls->sendRequest(didOpen);
        } else {
            setupEditor();
            files.append(fileName);
            QString name = path.right(path.size() - path.lastIndexOf('/')-1);
            tabWidget->addTab(currentEditor, name);
            QString text = file.readAll();
            currentEditor->setPlainText(text);
            QFileInfo info(fileName);
            if(!file.isWritable())
                currentEditor->setReadOnly(true);
        }
        tabWidget->setCurrentIndex(tabWidget->count()-1);
        currentEditor->setFocus();
    }
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    const QIcon newIcon = QIcon::fromTheme("document-new");
    QAction *newAct = new QAction(newIcon, tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newProject);
    fileMenu->addAction(newAct);
    fileToolBar->addAction(newAct);

    const QIcon openIcon = QIcon::fromTheme("document-open");
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save");
    QAction *saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    QAction *saveAsAct = fileMenu->addAction(saveAsIcon, tr("Save &As..."), this, &MainWindow::saveAs);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));

    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));


    QMenu *buildMenu = menuBar()->addMenu(tr("&Build"));
    const QIcon buildIcon = QIcon(":/images/build.png");
    QAction *buildAct = new QAction(buildIcon, tr("Build project"), this);
    buildAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(buildAct, &QAction::triggered, this, &MainWindow::build);
    buildMenu->addAction(buildAct);
    fileToolBar->addAction(buildAct);

    QMenu *debugMenu = menuBar()->addMenu(tr("&Debug"));
    const QIcon debugIcon = QIcon(":/images/debug.png");
    QAction *debugAct = new QAction(debugIcon, tr("Debug project"), this);
    debugAct->setShortcut(QKeySequence(Qt::Key_F5));
    connect(debugAct, &QAction::triggered, this, &MainWindow::debug);
    debugMenu->addAction(debugAct);
    fileToolBar->addAction(debugAct);


    const QIcon debugStopIcon = QIcon(":/images/stop.png");
    QAction *debugStopAct = new QAction(debugStopIcon, tr("&Stop debugger"), this);
    debugStopAct->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F5));
    connect(debugStopAct, &QAction::triggered, this, &MainWindow::stopDebug);
    debugMenu->addAction(debugStopAct);
    fileToolBar->addAction(debugStopAct);

    debugMenu->addSeparator();

    const QIcon runToIcon = QIcon(":/images/runto.png");
    QAction *runToAct = new QAction(runToIcon, tr("Run to cursor"), this);
    runToAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F10));
    connect(runToAct, &QAction::triggered, this, &MainWindow::runToLine);
    debugMenu->addAction(runToAct);
    fileToolBar->addAction(runToAct);

    const QIcon nextIcon = QIcon(":/images/next.png");
    QAction *nextAct = new QAction(nextIcon, tr("Next line"), this);
    nextAct->setShortcut(QKeySequence(Qt::Key_F10));
    connect(nextAct, &QAction::triggered, this, &MainWindow::nextLine);
    debugMenu->addAction(nextAct);
    fileToolBar->addAction(nextAct);

    const QIcon intoIcon = QIcon(":/images/stepin.png");
    QAction *intoAct = new QAction(intoIcon, tr("Step into"), this);
    intoAct->setShortcut(QKeySequence(Qt::Key_F11));
    connect(intoAct, &QAction::triggered, this, &MainWindow::stepInto);
    debugMenu->addAction(intoAct);
    fileToolBar->addAction(intoAct);

    const QIcon outIcon = QIcon(":/images/stepout.png");
    QAction *outAct = new QAction(outIcon, tr("Step out"), this);
    outAct->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F11));
    connect(outAct, &QAction::triggered, this, &MainWindow::stepOut);
    debugMenu->addAction(outAct);
    fileToolBar->addAction(outAct);

    debugMenu->addSeparator();

    QAction *toggleAct = new QAction(tr("Toggle breakpoint"), this);
    toggleAct->setShortcut(QKeySequence(Qt::Key_F9));
    connect(toggleAct, &QAction::triggered, this, &MainWindow::breakPoint);
    debugMenu->addAction(toggleAct);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));
}

void MainWindow::breakPoint() {
    if (currentEditor) {
        int bn = currentEditor->textCursor().blockNumber();
        int index = currentEditor->breakpoints.indexOf(bn);
        if (index != -1)
            currentEditor->breakpoints.remove(index);
        else
            currentEditor->breakpoints.append(bn);
    }
}

void MainWindow::saveAs()
{
    if (currentEditor) {
        currentEditor->saveAs();
    }
}

void MainWindow::open()
{
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
            loadFile(fileName);
}

QStringList separate(const QString &s)
{
    QStringList list;
    QString current;
    int brackets = 0;
    for (int i = 0; i < s.size(); i++) {
        QChar c = s[i];
        if (c == '{') {
            brackets++;
        } else if (c == '}') {
            brackets--;
        } else if (c == ',' && brackets == 0) {
            list.append(current);
            current.clear();
            i++;
            continue;
        }
        current.append(c);
    }
    list.append(current);
    return list;
}

void getLocals(int depth, QStringList &list, const QString &vs)
{
    QStringList sl = separate(vs);

    for (const auto &s: sl) {
        std::cerr << s.toStdString() << '\n';
        int start = s.indexOf('{');
        QString indent;
        for (int i = 0; i < depth; i++)
            indent.push_back(' ');
        if (start != -1) {
            int equal = s.indexOf("= ");
            QString v = s.left(equal+2);
            indent.append(v);
            list.append(indent);
            QString mid = s.mid(start+1, s.size()-start-2);
            getLocals(depth + 1, list, mid);
        } else {
            list.append(indent + s);
        }
    }
}


void MainWindow::readDebugOutput()
{
    QString s = db->readAllStandardOutput();
    QRegularExpression startExpression("Starting program");
    QRegularExpression endExpression("\\[Inferior 1 \\(process [0-9]+\\) exited normally\\]");
    QRegularExpression bpExpression("[^\\s]+:[0-9]+");
    QRegularExpression numExpression("^[0-9]+");
    QRegularExpression runExpression("^Run till exit from ");
    std::cerr << '\"' << s.toStdString() << "\"" << '\n';

    if (tempFile.open()) {
        int fileSize = tempFile.size();
        if (fileSize > outFileSize) {
            tempFile.seek(outFileSize);
            outFileSize = fileSize;
            QString text = tempFile.readAll();
            text = text.left(text.size()-1);
            applicationOutput->appendPlainText(text);
        }
    }

    if (runExpression.match(s).hasMatch()) {
        return;
    } else if (startExpression.match(s).capturedStart() != -1) {
        dbStatus = Db::started;
    } else if (endExpression.match(s).capturedStart() != -1) {
        db->write("quit\n");
        dbStatus = Db::none;
        currentEditor->stepNumber = -1;
        currentEditor->update();
        applicationOutput->appendPlainText(getTime() + "Debugging ended\n");
        return;
    }
    QRegularExpressionMatch bpMatch = bpExpression.match(s);
    int end = bpMatch.capturedEnd();
    QRegularExpressionMatch numMatch = numExpression.match(s);
    int nend = numMatch.capturedEnd();
    if (dbStatus == Db::started && (end != -1 || nend != -1)) {
        if (end != -1) {
            QString captured = bpMatch.captured();
            int colon = captured.indexOf(':');
            QStringRef name = captured.leftRef(colon);
            QStringRef num = captured.rightRef(captured.size()-colon-1);
            currentEditor->stepNumber = num.toInt()-1;
        } else if (nend != -1) {
            currentEditor->stepNumber = numMatch.captured().toInt()-1;
        }
        currentEditor->update();
        db->write("info locals\n");
        dbStatus = Db::locals;
    } else if (dbStatus == Db::locals) {
        QStringList locals = s.split('\n');
        locals.pop_back();
        QStringList list;
        for (const auto &local: locals) {
            int start = local.indexOf('{');
            if (start == -1) {
                list.append(local);
            } else {
                int n = local.indexOf('=');
                list.append(local.left(n+2));
                QString mid = local.mid(start+1, local.size()-start-2);
                mid.replace(": ", " = ");
                getLocals(1, list, mid);
            }
        }

        nodeModel->setModelData(list);
        dbStatus = Db::interrupted;
    } else if (dbStatus == Db::interrupted) {
        QRegularExpression numExpression("(?<=at )[^\n]+:[0-9]+");
        QRegularExpressionMatch numMatch = numExpression.match(s);
        if (numMatch.hasMatch()) {
            QStringList list = numMatch.captured().split(':');
            QString fileName = rls->dirName + '/' + list.at(0);
            if (!files.contains(fileName)) {
                int index = fileName.indexOf('/');
                if (fileName.left(index) != "..") {
                    loadFile(fileName);
                    currentEditor->stepNumber = list.at(1).toInt();
                } else {
                    QProcess p;
                    QStringList args(fileName.right(index));
                    p.start("/usr/bin/locate", args);
                    p.waitForFinished();
                    QString result = p.readAllStandardOutput();
                    if (!result.isEmpty()) {
                        loadFile(result.left(result.indexOf('\n')));
                        currentEditor->stepNumber = list.at(1).toInt();
                    } else {
                        db->write("finish\n");
                        return;
                    }
                }
            }
        }
        db->write("info locals\n");
        dbStatus = Db::locals;
    }
}

void MainWindow::setupDockWindows()
{
    QDockWidget *dock = new QDockWidget(tr("Logs"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);
    logs = new QTabWidget(dock);
    issues = new QPlainTextEdit(dock);
    issues->document()->setMaximumBlockCount(100);
    issues->setReadOnly(true);
    logs->addTab(issues, "Issues");
    applicationOutput = new QPlainTextEdit(dock);
    applicationOutput->document()->setMaximumBlockCount(100);
    applicationOutput->setReadOnly(true);
    logs->addTab(applicationOutput, "Application Output");
    compileOutput = new QPlainTextEdit(dock);
    compileOutput->document()->setMaximumBlockCount(100);
    compileOutput->setReadOnly(true);
    logs->addTab(compileOutput, "Compile Output");
    dock->setWidget(logs);
    addDockWidget(Qt::BottomDockWidgetArea, dock);

    connect(db, &QProcess::readyReadStandardOutput, this, &MainWindow::readDebugOutput);

    QMenu *viewMenu = new QMenu(tr("&View"), this);
    menuBar()->addMenu(viewMenu);
    viewMenu->addAction(dock->toggleViewAction());

    dirModel = new QFileSystemModel(this);

    if (!tempDir.isValid()) {

        throw "Error with temporary directory";
    }

    dirModel->setRootPath(tempDir.path());

    dock = new QDockWidget(tr("Management"), this);
    management = new QTabWidget(dock);
    treeView = new QTreeView(management);
    treeView->setModel(dirModel);
    QModelIndex rootIndex = dirModel->index(tempDir.path());
    treeView->setRootIndex(rootIndex);
    treeView->setHeaderHidden(true);
    for (int i = 1; i < 4; i++)
        treeView->hideColumn(i);
    management->addTab(treeView, "Project");
    nodeModel = new NodeModel("", management);
    variableView = new QTreeView(management);
    variableView->setModel(nodeModel);
    management->addTab(variableView, "Variables");
    dock->setWidget(management);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());

}

void MainWindow::setupEditor()
{
    QFont font;
    font.setFamily("Source Code Pro");
    font.setFixedPitch(true);
    font.setPointSize(10);

    currentEditor = new CodeEditor(nullptr, rls);
    undoGroup->addStack(currentEditor->undoStack);
    currentEditor->setFont(font);

    new Highlighter(currentEditor->document());

    connect(currentEditor->undoStack, &QUndoStack::indexChanged,
                this, &MainWindow::documentWasModified);
}

void MainWindow::documentWasModified()
{
    QString title = currentEditor->fileName;
    if(currentEditor->undoStack->index() != currentEditor->undoIndex) {
        title += "*";
        tabWidget->setTabText(tabWidget->currentIndex(), title.toStdString().c_str());
    }
    else
        tabWidget->setTabText(tabWidget->currentIndex(), title.toStdString().c_str());
}

void MainWindow::setupFileMenu()
{
    QMenu *fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);

    fileMenu->addAction(tr("&New"), this, SLOT(newFile()), QKeySequence::New);
    fileMenu->addAction(tr("&Open..."), this, SLOT(openFile()), QKeySequence::Open);
    fileMenu->addAction(tr("&Save"), this, SLOT(saveFile()), QKeySequence::Save);
    fileMenu->addAction(tr("E&xit"), qApp, SLOT(quit()), QKeySequence::Quit);
}

void MainWindow::setupHelpMenu()
{
    QMenu *helpMenu = new QMenu(tr("&Help"), this);
    menuBar()->addMenu(helpMenu);

    helpMenu->addAction(tr("&About"), this, SLOT(about()));
}
