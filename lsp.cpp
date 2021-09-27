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

#include "lsp.h"

Client::Client(QObject *parent, int pid, QString dirName): dirName(dirName), parent(parent), pid(pid), rootUri("file://" + dirName), cv(2048) {
    lengthExpression = QRegularExpression("Content-Length: ");
    terminateExpression = QRegularExpression("\r\n\r\n");
    start();
}

void Client::start() {
    idNumber = 0;
    length = 0;
    ls = new QProcess(parent);
    ls->setProgram("rls");
    if (!ls->open()) {
        throw "Error starting rls";
    }
    QJsonObject params;
    params.insert("processId", pid);
    params.insert("rootUri", rootUri);
    QJsonObject capabilities;
    params.insert("capabilities", capabilities);
    QJsonObject initRequest = createRequest("initialize", params);
    sendRequest(initRequest);
    bool initialized = false;
    if (ls->waitForReadyRead()) {
        QJsonObject initResponse = getResponses()[0];
        if (initResponse.contains("result")) {
            QJsonObject initNotification = createRequest("initialized", QJsonObject());
            sendRequest(initNotification);
            initialized = true;
            QVector<QString> ids;
            bool b = false;
            while (ls->waitForReadyRead()) {
                QVector<QJsonObject> responses = getResponses();
                for (const auto &response: responses) {
                    if (response.value("method") == "client/registerCapability") {
                        b = true;
                    }
                }
                if (b)
                    break;
            }
        }
    }
    if (!initialized) {
        throw "Error initializing rls";
    }
}

Client::~Client() {
    keep = false;
    if (ls) {
        QJsonObject params;
        QJsonObject shutdownRequest = createRequest("shutdown", params);
        sendRequest(shutdownRequest);
                    QJsonObject exitNotification = createRequest("exit", QJsonObject());
                    sendRequest(exitNotification);
                        return;
    }
    std::cerr <<  "Error shutting down rls\n";
}


void Client::sendRequest(const QJsonObject &request) {
        QJsonDocument doc(request);
        QByteArray content = doc.toJson(QJsonDocument::Compact);
        QString length = QString::number(content.size());
        QString s = "Content-Length: " + length + "\r\n\r\n";
        QByteArray header = s.toUtf8();
        header.append(content);
        int written = ls->write(header);
        if (written == -1)
            throw "Error sending request";
}

QVector<QJsonObject> Client::getResponses() {
    QString out = ls->readAllStandardOutput();
    bool b = false;
    if (!old.isEmpty()) {
        out = old + out;
        old.clear();
        b = true;
    }
    int sz = out.size();
    int n = 0;
    QVector<QJsonObject> responses;
    while (n < sz) {
        QJsonObject o;
        QRegularExpressionMatch match = lengthExpression.match(out, n);
        int captured = match.capturedEnd();
        if (captured != -1) {
            int end = terminateExpression.match(out, captured).capturedStart();
            if (end != -1) {
                length = out.mid(captured, end-captured).toInt();
                if(b){
                    length -= 4;
                }
                int l2 = sz - end - 4;
                if (length <= l2) {
                    QString q = out.mid(end+4, length);
                    o = QJsonDocument::fromJson(q.toUtf8()).object();
                    if (o.empty())
                        throw "Error parsing response";
                    int total = length+end+4-n;
                    n += total;
                }
                else if (l2 == 0)
                    return responses;
                else {
                    old = out;
                    return responses;
                }
            } else {
                throw "Error parsing response";
            }
        } else {
            QString q = out.mid(n, length);
            o = QJsonDocument::fromJson(q.toUtf8()).object();
            if (o.empty())
                throw "Error getting response";
            int total = length;
            n += total;
        }
        if (o.empty())
            throw "Error getting response";
        responses.push_back(o);
    }
    length = 0;
    return responses;
}

QJsonObject Client::createRequest(const QString &method, const QJsonObject &params) {
    QJsonObject o;
    o.insert("jsonrpc", "2.0");
    o.insert("id", idNumber);
    idNumber++;
    o.insert("method", method);
    o.insert("params", params);
    return o;
}
