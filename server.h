#ifndef SERVER_H
#define SERVER_H

#include "user.h"
#include "def.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QThread>

class Server : public QTcpServer{
    Q_OBJECT
public:    
    explicit Server(QObject *parent = nullptr);
    QHash<qintptr, User*>* getConnections();

private:
    void incomingConnection(qintptr handle);
    QHash<qintptr, User*> connections;
    QHash<QString, QString> registrationList;
    QHash<QString, QString> recoveryList;

private slots:
    void deleteConnection(qintptr);
    void dispatchingMessage();
    void newRegistration(QString, QString);
    void registrationCode(QJsonObject);
    void newRecovery(QString, QString);
    void recoveryCode(QString, QString);
    void recoveryNewPass(QString, QString);

signals:
    void startDispatching();
};

#endif // SERVER_H
