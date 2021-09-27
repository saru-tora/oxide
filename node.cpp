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

#include "node.h"
#include <iostream>

Node::Node(const QVector<QVariant> &data, Node *parent)
    : m_data(data), m_parent(parent)
{}

Node::~Node()
{
    qDeleteAll(m_children);
}

void Node::appendChild(Node *node)
{
    m_children.append(node);
}

Node *Node::child(int row)
{
    if (row < 0 || row >= m_children.size())
        return nullptr;
    return m_children.at(row);
}

int Node::childCount() const
{
    return m_children.count();
}

int Node::columnCount() const
{
    return m_data.count();
}

QVariant Node::data(int column) const
{
    if (column < 0 || column >= m_data.size())
        return QVariant();
    return m_data.at(column);
}

Node *Node::parent()
{
    return m_parent;
}

int Node::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<Node*>(this));
    return 0;
}

FileNode::FileNode(const QString &directory, FileNode *parent)
    : m_parent(parent)
{
    if (!directory.isEmpty()) {
        m_data = new QFileSystemModel;
        m_data->setRootPath(directory);
    } else
        m_data = nullptr;
}

FileNode::~FileNode()
{
    if(m_data)
        delete m_data;
    qDeleteAll(m_children);
}

void FileNode::appendChild(FileNode *node)
{
    m_children.append(node);
}

FileNode *FileNode::child(int row)
{
    if (row < 0 || row >= m_children.size())
        return nullptr;
    return m_children.at(row);
}

int FileNode::childCount() const
{
    return m_children.count();
}

int FileNode::columnCount() const
{
    return 1;
}

QFileSystemModel *FileNode::data(int column) const
{
    if (column < 0 || column > 1)
        throw "Out of bounds";
    return m_data;
}

FileNode *FileNode::parent()
{
    return m_parent;
}

int FileNode::row() const
{
    if (m_parent)
        return m_parent->m_children.indexOf(const_cast<FileNode*>(this));
    return 0;
}
