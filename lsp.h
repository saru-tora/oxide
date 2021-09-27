#ifndef LSP_H
#define LSP_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <iostream>

class Client {
public:
    Client(QObject *parent, int pid, QString dirName);
    Client(Client&) = delete;
    ~Client();
    QJsonObject createRequest(const QString &method, const QJsonObject &params);
    void sendRequest(const QJsonObject &request);
    void start();
    QVector<QJsonObject> getResponses();
    QProcess *ls = nullptr;
    bool keep = false;
    QString dirName;
private:
    QObject *parent;
    int pid;
    QString rootUri;
    QString old;
    QRegularExpression lengthExpression;
    QRegularExpression terminateExpression;
    int idNumber = 0;
    int length = 0;
    QVector<char> cv;
};

#endif // LSP_H
