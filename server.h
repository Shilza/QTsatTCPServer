#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QThread>
#include "connection.h"
#include "messagesender.h"
#include "def.h"


class Server : public QTcpServer{
    Q_OBJECT
public:    
    explicit Server(QObject *parent = nullptr);

private:
    void incomingConnection(qintptr handle);
    QHash<qintptr, Connection*> connections;
    MessageSender messageSender;
    QThread senderThread;

private slots:
    void deleteConnection(qintptr);
    void dispatchingMessage();
};

#endif // SERVER_H
