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

#include "commands.h"

QTextCursor getRight(CodeEditor* editor) {
    QTextCursor tc = editor->textCursor();
    int pos = editor->textCursor().position();
    int anch = editor->textCursor().anchor();
    if (pos < anch) {
        tc.clearSelection();
        tc.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, anch - pos);
    }
    return tc;
}

AddCommand::AddCommand(CodeEditor* editor, QString text, QString old, bool add, QUndoCommand *parent)
    : QUndoCommand(parent), editor(editor), s(text), old(old), tc(getRight(editor)), add(add)
{
}

bool AddCommand::mergeWith(const QUndoCommand *command)
{
    const AddCommand *addCommand = static_cast<const AddCommand *>(command);

    bool a = !addCommand->old.isEmpty();
    bool b = tc.position() + s.size() != editor->textCursor().position()+1;
    bool c = false;
    if (!s.isEmpty())
        c = s.at(s.size()-1) == '\n';
    if (a || b || c)
        return false;

    s.append(addCommand->s);

    return true;
}

void AddCommand::undo()
{
    editor->sendChange(tc, old, s.size());
}

void AddCommand::redo()
{
    editor->sendChange(tc, s, old.size(), add);
}

DeleteCommand::DeleteCommand(CodeEditor* editor, QString text, QUndoCommand *parent)
    : QUndoCommand(parent), editor(editor), s(text), tc(getRight(editor))
{
}

bool DeleteCommand::mergeWith(const QUndoCommand *command)
{
    const DeleteCommand *deleteCommand = static_cast<const DeleteCommand *>(command);

    s.append(deleteCommand->s);

    return true;
}

void DeleteCommand::undo()
{
    editor->sendChange(tc, s, 0);
}

void DeleteCommand::redo()
{
    editor->sendChange(tc, "", 1);
}

RemoveCommand::RemoveCommand(CodeEditor* editor, QString text, QUndoCommand *parent)
    : QUndoCommand(parent), editor(editor), s(text), tc(getRight(editor))
{
}

void RemoveCommand::undo()
{
    editor->sendChange(tc, s, 0);
}

void RemoveCommand::redo()
{
    QString e = "";
    editor->sendChange(tc, e, s.size());
}
