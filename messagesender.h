#ifndef SENDER_H
#define SENDER_H

#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include "server.h"

class MessageSender : public QThread{
    Q_OBJECT
public:
    MessageSender();

private:
    void run() override;
    quint64 lastMessage = 0;
    Server server;

public slots:
    void start();
};

#endif // SENDER_H
