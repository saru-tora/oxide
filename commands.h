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

#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include <QTextCursor>

#include "codeeditor.h"

class AddCommand : public QUndoCommand
{
public:
    enum { Id = 1 };

    AddCommand(CodeEditor* editor, QString text, QString old = "", bool add = true, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return Id; }

private:
    CodeEditor* editor;
    QString s;
    QString old;
    QTextCursor tc;
    bool add;
};

class DeleteCommand : public QUndoCommand
{
public:
    enum { Id = 2 };

    explicit DeleteCommand(CodeEditor* editor, QString text, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;
    bool mergeWith(const QUndoCommand *command) override;
    int id() const override { return Id; }

private:
    CodeEditor* editor;
    QString s;
    QTextCursor tc;
};

class RemoveCommand : public QUndoCommand
{
public:
    RemoveCommand(CodeEditor* editor, QString text, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    CodeEditor* editor;
    QString s;
    QTextCursor tc;
};

#endif // COMMANDS_H
