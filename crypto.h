#ifndef CRYPTO_H
#define CRYPTO_H

#include <QCryptographicHash>
#include <QDateTime>
#include <QSqlQuery>
#include <QPair>

typedef QString Salt;
typedef QString Password;

const QByteArray SALT = "%%FJ$UW)TЖT^@(Ъ";

QPair<Salt, Password> hashPassword(QString password, uint date);
QString hashPassword(QString login, QString password);

#endif // CRYPTO_H
