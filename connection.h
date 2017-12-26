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
#include "smtp.h"
#include "def.h"

class Connection : public QObject{
    Q_OBJECT
public:
    Connection(qintptr handle, QObject *parent = nullptr);
    void send(QJsonDocument message);

private:
    QTcpSocket *socket;
    QString nickname;
    QString location;
    QQueue<QString> lastMessages;
    quint8 floodCounter = 0;
    int floodTimer = 0;

    QJsonObject authorization(QJsonObject);
    QJsonObject registration(QJsonObject);
    QJsonObject registrationCode(QJsonObject);
    QJsonObject recovery(QJsonObject);
    QJsonObject recoveryCode(QJsonObject);
    QJsonObject recoveryNewPass(QJsonObject);
    QJsonObject doesNicknameExist(QJsonObject);
    QJsonObject doesEmailExist(QJsonObject);
    QJsonObject sendGlobalMessage(QJsonObject);

signals:
    void disconnected(qintptr);
    void dispatchMessage();
    void passwordChanged(QString, QString);

private slots:
    void disconnecting();
    void controller();
    void sendEmailPasswordChanged(QString email, QString nickname);
};

#endif // USER_H
