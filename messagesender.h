#ifndef SENDER_H
#define SENDER_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include "connection.h"

class MessageSender : public QObject{
    Q_OBJECT
public:
    MessageSender();
    void setConnections(QHash<qintptr, Connection *> *connections);

public slots:
    void start();

private:
    quint64 lastMessage = 0;
    QHash<qintptr, Connection*> *connections;

signals:
    void finished();
};

#endif // SENDER_H
