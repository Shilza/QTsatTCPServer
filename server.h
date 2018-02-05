#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
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
    QTcpSocket *ftpSocket;

private slots:
    void deleteConnection(qintptr);
    void dispatchingMessage();
    void ftpController();
};

#endif // SERVER_H
