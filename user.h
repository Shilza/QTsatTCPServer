#ifndef USER_H
#define USER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSqlQuery>
#include <QDateTime>
#include <QQueue>
#include <QCryptographicHash>
#include "distance_damerau_levenshtein.h"
#include "def.h"

class User : public QObject{
    Q_OBJECT
public:
    User(qintptr handle, QObject *parent = nullptr);
    void send(QJsonDocument message);
    QTcpSocket* getSocket();

private:
    QTcpSocket *socket;
    QString nickname;
    QString location;
    QQueue<QString> lastMessages;
    quint8 floodCounter = 0;
    int floodTimer = 0;

signals:
    void disconnected(qintptr);
    void dispatchMessage();
    void newRegistration(QString, QString);
    void registrationCode(QJsonObject);
    void newRecovery(QString, QString);
    void recoveryCode(QString, QString);
    void recoveryNewPass(QString, QString);

private slots:
    void disconnecting();
    void reading();
};

#endif // USER_H
