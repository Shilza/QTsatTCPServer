#include "user.h"


User::User(qintptr handle, QObject *parent) : QObject(parent){
    socket = new QTcpSocket(this);
    socket->setSocketDescriptor(handle);
    location = "";

    lastMessages.clear();

    connect(socket, SIGNAL(disconnected()), SLOT(disconnecting()));
    connect(socket, SIGNAL(readyRead()), SLOT(reading()));
}

void User::send(QJsonDocument message){
    socket->write(message.toJson());
}

QTcpSocket* User::getSocket(){
    return socket;
}

void User::disconnecting(){
    emit disconnected(socket->socketDescriptor());
    socket->deleteLater();
}

void User::reading(){
    QByteArray receivedObject = socket->readAll();
    QJsonParseError error;

    QJsonObject request = QJsonDocument::fromJson(receivedObject, &error).object();
    QJsonObject response;
    if(error.error == QJsonParseError::NoError){
        if(request.value("Target").toString() == "Authorization"){
            response.insert("Target", "Authorization");

            QSqlQuery query;
            query.prepare("SELECT ID FROM users WHERE (Nickname=? OR Email=?) AND Password=?");
            query.bindValue(0, request.value("Login").toString());
            query.bindValue(1, request.value("Login").toString());
            query.bindValue(2, request.value("Password").toString());
            query.exec();

            QString id="";
            while (query.next())
                id = query.value(0).toString();

            if(id != ""){
                nickname = request.value("Login").toString();
                response.insert("Value", "Authorization successful");
            }
            else
                response.insert("Value", "Authorization failed");

        }
        else if(request.value("Target").toString() == "Registration"){
            response.insert("Target", "Registration");

            QSqlQuery queryExist;
            queryExist.prepare("SELECT ID FROM users WHERE Email=? OR Nickname=?");
            queryExist.bindValue(0, request.value("Email").toString());
            queryExist.bindValue(1, request.value("Nickname").toString());
            queryExist.exec();

            QString id="";
            while (queryExist.next())
                id = queryExist.value(0).toString();

            if(id==""){
                emit newRegistration(request.value("Email").toString(), QString().append(QCryptographicHash::hash(QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentDateTime().toTime_t()), QCryptographicHash::Md5).toHex()).left(6));
                response.insert("Value", "Email doesn't exist");
                //DO SEND EMAIL
            }
            else
                response.insert("Value", "Email exists");
        }
        else if(request.value("Target").toString() == "Registration code"){
            emit registrationCode(request);
            return;
        }
        else if(request.value("Target").toString() == "Recovery"){
            response.insert("Target", "Recovery");

            QSqlQuery query;
            query.prepare("SELECT Email FROM users WHERE Email=? OR Nickname=?");
            query.bindValue(0, request.value("Value"));
            query.bindValue(1, request.value("Value"));
            query.exec();

            QString email="";
            while (query.next())
                email = query.value(0).toString();

            if(email!=""){
                //DO SEND EMAIL
                emit newRecovery(email, QString().append(QCryptographicHash::hash(QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentDateTime().toTime_t()), QCryptographicHash::Md5).toHex()).mid(0,6));
                response.insert("Value", "Founded");
            }
            else
                response.insert("Value", "Not founded");
        }
        else if(request.value("Target").toString() == "Recovery code"){
            QSqlQuery queryEmail;
            queryEmail.prepare("SELECT Email FROM users WHERE Email=? OR Nickname=?");
            queryEmail.bindValue(0, request.value("Value").toString());
            queryEmail.bindValue(1, request.value("Value").toString());
            queryEmail.exec();

            QString email="";
            while (queryEmail.next())
                email = queryEmail.value(0).toString();

            emit recoveryCode(email, request.value("Code").toString());
            return;
        }
        else if(request.value("Target").toString() == "Recovery new pass"){
            QSqlQuery queryEmail;
            queryEmail.prepare("SELECT Email FROM users WHERE Email=? OR Nickname=?");
            queryEmail.bindValue(0, request.value("Value").toString());
            queryEmail.bindValue(1, request.value("Value").toString());
            queryEmail.exec();

            QString email="";
            while (queryEmail.next())
                email = queryEmail.value(0).toString();

            emit recoveryNewPass(email, request.value("Password").toString());
            return;
        }
        else if(request.value("Target").toString() == "PMessage"){
            //TODO
        }
        else if(request.value("Target").toString() == "GMessage"){
            if(QDateTime::currentDateTime().toTime_t() < floodTimer)
                return;

            response.insert("Target", "Message status");

            QString text = request.value("message").toString();

            if(text.indexOf('\n')!=-1){
                while(text.indexOf("  ") != -1)
                    text = text.remove(text.indexOf("  "), 2).insert(text.indexOf("  "), " ");

                text = text.simplified().insert(text.indexOf('\n'), '\n');
            }
            else
                text = text.simplified();

            if(text.length()>MAX_GLOBAL_MESSAGE_SIZE)
                text = text.left(MAX_GLOBAL_MESSAGE_SIZE);

            bool isMessageGoingToBeSended = true;
            if(lastMessages.size()>=3){
                QVector<quint8> levenshteinDistances;
                for(QString comparisonString : lastMessages)
                    levenshteinDistances.push_back(levenshteinDistance(text.toStdString(), comparisonString.toStdString()));
                quint8 count=0;
                for(quint8 temp : levenshteinDistances){
                    if(temp<=(text.length()/2 < 1 ? 1 : text.length()/2))
                        count++;
                }
                if(count>=3 && floodCounter<3){
                    floodCounter++;
                    floodTimer = QDateTime::currentDateTime().toTime_t()+3000*floodCounter;
                    response.insert("Value", "Message not delivered");
                    response.insert("Cause", "Flood");
                    response.insert("Time", floodTimer);
                    isMessageGoingToBeSended = false;
                }
                else if(count>=3){
                    QSqlQuery query;
                    query.prepare("UPDATE users SET 'Last ban' = ? WHERE Nickname = ?");
                    query.bindValue(0, QDateTime::currentDateTime().toTime_t()+14400);//4 hours for flood
                    query.bindValue(1, nickname);
                    query.exec();

                    //for bans history
                    query.prepare("INSERT INTO users (Nickname, Message, Start time, Finish time, Cause) VALUES = (?, ?, ?, ?, 'Flood')");
                    query.bindValue(0, nickname);
                    query.bindValue(1, text);
                    query.bindValue(2, QDateTime::currentDateTime().toTime_t());
                    query.bindValue(3, QDateTime::currentDateTime().toTime_t()+14400);
                    query.exec();

                    response.insert("Value", "Ban");
                    response.insert("Time", int(QDateTime::currentDateTime().toTime_t()+14400));

                    floodCounter = 0;
                    lastMessages.clear();
                    isMessageGoingToBeSended = false;
                }
            }
            if(isMessageGoingToBeSended){
                if(lastMessages.size()==5)
                    lastMessages.pop_front();
                lastMessages.push_back(text);

                QSqlQuery query;
                query.prepare("INSERT INTO messages (Sender, Text, Time) VALUES (:sender, :text, :time)");
                query.bindValue(":sender", nickname);
                query.bindValue(":text", text);
                query.bindValue(":time", QDateTime::currentDateTime().toTime_t());
                if(query.exec())
                    emit dispatchMessage();
            }
        }
        else if(request.value("Target").toString() == "Post"){

        }
        else if(request.value("Target").toString() == "DoesNicknameExist"){
            response.insert("Target", "DoesNicknameExist");

            QSqlQuery query;
            query.prepare("SELECT ID FROM users WHERE Nickname=?");
            query.bindValue(0, request.value("Nickname").toString());
            query.exec();

            QString id="";
            while (query.next())
                id = query.value(0).toString();

            response.insert("Value", id != "" ? "Nickname exist" : "Nickname doesn't exist");
        }
    }
    socket->write(QJsonDocument(response).toJson());
}
