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

#include "nodemodel.h"
#include "node.h"
#include <iostream>

#include <QStringList>

NodeModel::NodeModel(const QString &data, QObject *parent)
    : QAbstractItemModel(parent)
{
    root = new Node({tr("Name"), tr("Value")});
    setupModelData(data.split('\n'), root);
}

NodeModel::~NodeModel()
{
    delete root;
}

void NodeModel::setModelData(const QStringList &data)
{
    delete root;
    root = new Node({tr("Name"), tr("Value")});
    setupModelData(data, root);
}

int NodeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<Node*>(parent.internalPointer())->columnCount();
    return root->columnCount();
}

QVariant NodeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    Node *item = static_cast<Node*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags NodeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant NodeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return root->data(section);

    return QVariant();
}

QModelIndex NodeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Node *parentItem;

    if (!parent.isValid())
        parentItem = root;
    else
        parentItem = static_cast<Node*>(parent.internalPointer());

    Node *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex NodeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Node *child = static_cast<Node*>(index.internalPointer());
    Node *parent = child->parent();

    if (parent == root)
        return QModelIndex();

    return createIndex(parent->row(), 0, parent);
}

int NodeModel::rowCount(const QModelIndex &parent) const
{
    Node *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = root;
    else
        parentItem = static_cast<Node*>(parent.internalPointer());

    return parentItem->childCount();
}

void NodeModel::setupModelData(const QStringList &lines, Node *parent)
{
    QVector<Node*> parents;
    QVector<int> indentations;
    parents << parent;
    indentations << 0;

    int number = 0;

    while (number < lines.count()) {
        int position = 0;
        while (position < lines[number].length()) {
            if (lines[number].at(position) != ' ')
                break;
            position++;
        }

        const QString lineData = lines[number].mid(position).trimmed();

        if (!lineData.isEmpty()) {
            const QStringList columnStrings =
                lineData.split(QLatin1Char('='));
            QVector<QVariant> columnData;
            columnData.reserve(columnStrings.count());
            for (const QString &columnString : columnStrings) {
                columnData << columnString;
            }

            if (position > indentations.last()) {

                if (parents.last()->childCount() > 0) {
                    parents << parents.last()->child(parents.last()->childCount()-1);
                    indentations << position;
                }
            } else {
                while (position < indentations.last() && parents.count() > 0) {
                    parents.pop_back();
                    indentations.pop_back();
                }
            }

            parents.last()->appendChild(new Node(columnData, parents.last()));
        }
        ++number;
    }
}

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    root = new FileNode("");
}

ProjectModel::~ProjectModel()
{
    delete root;
}

int ProjectModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<FileNode*>(parent.internalPointer())->columnCount();
    return root->columnCount();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    FileNode *item = static_cast<FileNode*>(index.internalPointer());

    return item->data(index.column())->data(index);
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QModelIndex ProjectModel::index(const QString &directory, int column) const
{
    for (int i = 0; i < root->childCount(); i++) {
        FileNode *child = root->child(i);
        if (child->data(0)->rootPath() == directory)
            return createIndex(i, 1, child);
    }
    if (column > 0)
        throw "index error";
    return QModelIndex();
}

QModelIndex ProjectModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column > 0)
        return QModelIndex();

    for (int i = 0; i < root->childCount(); i++) {
        FileNode *child = root->child(i);
        if (child->data(0)->rootPath() == static_cast<FileNode*>(parent.internalPointer())->data(0)->rootPath())
            return createIndex(i, 0, child);
    }

    return QModelIndex();
}

QModelIndex ProjectModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    FileNode *child = static_cast<FileNode*>(index.internalPointer());
    FileNode *parent = child->parent();

    if (parent == root)
        return QModelIndex();

    return createIndex(parent->row(), 0, parent);
}

int ProjectModel::rowCount(const QModelIndex &parent) const
{
    FileNode *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = root;
    else
        parentItem = static_cast<FileNode*>(parent.internalPointer());

    return parentItem->childCount();
}

void ProjectModel::setModelData(const QString &directory)
{
    root->appendChild(new FileNode(directory));
}
