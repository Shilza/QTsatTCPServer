#include "connection.h"
#include <QSqlError>
#include <QDebug>

Connection::Connection(qintptr handle, QObject *parent) : QObject(parent){
    socket = new QTcpSocket(this);
    socket->setSocketDescriptor(handle);

    lastMessages.clear();

    connect(socket, SIGNAL(disconnected()), SLOT(disconnecting()));
    connect(socket, SIGNAL(readyRead()), SLOT(controller()));
    connect(this, SIGNAL(passwordChanged(QString, QString)), SLOT(sendEmailPasswordChanged(QString, QString)));
}

void Connection::send(QJsonDocument message){
    qDebug() << message.toJson() << '\n';
    socket->write(message.toJson());
}

QString Connection::getLocation(){
    return location;
}

QJsonObject Connection::authorization(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Authorization");

    if(request.contains("Access token")){
        nickname = request.value("Nickname").toString();

        QSqlQuery query;
        query.prepare("SELECT TokenTime, RefreshToken, LastBan FROM users WHERE AccessToken = ? AND Nickname = ?");
        query.addBindValue(request.value("Access token").toString());
        query.addBindValue(nickname);
        query.exec();

        while(query.next()){
            tokenTime = query.value(0).toInt();
            refreshToken = query.value(1).toString();
            banFinish = query.value(2).toInt();
        }

        if(tokenTime <= int(QDateTime::currentDateTime().toTime_t())){
            response.erase(response.find("Target"));
            response.insert("Target", "Token refreshing");
        }
        else{
            if(location == "")
                location = "GlobalChat";

            accessToken = request.value("Access token").toString();
            response.insert("Value", "Authorization successful");

            if(banFinish < QDateTime::currentDateTime().toTime_t())
                response.insert("Ban", int(banFinish));
        }
    }
    else{
        QSqlQuery query;
        query.prepare("SELECT TokenTime, Nickname, RefreshToken FROM users WHERE (Nickname=? OR Email=?) AND Password=?");
        query.bindValue(0, request.value("Login").toString());
        query.bindValue(1, request.value("Login").toString());
        query.bindValue(2, hashPassword(request.value("Login").toString(), request.value("Password").toString()));
        query.exec();

        QString nickname;
        while (query.next()){
            tokenTime = query.value(0).toInt();
            nickname = query.value(1).toString();
            refreshToken = query.value(2).toString();
        }

        if(tokenTime != -1){
            response.insert("Value", "Authorization successful");

            this->nickname = nickname;
            location = "GlobalChat";

            if(tokenTime <= int(QDateTime::currentDateTime().toTime_t())){
                accessTokenRefreshing();
                if(refreshToken == ""){
                    refreshToken = QString().append(QCryptographicHash::hash(accessToken.toUtf8() + QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentDateTime().toTime_t()), QCryptographicHash::Md5).toHex());

                    query.prepare("UPDATE users SET RefreshToken = ? WHERE Nickname = ?");
                    query.bindValue(0, refreshToken);
                    query.bindValue(1, nickname);
                    query.exec();
                }
            }
            else{
                query.prepare("SELECT AccessToken FROM users WHERE Nickname = ?");
                query.addBindValue(nickname);

                query.exec();
                while(query.next())
                    accessToken = query.value(0).toString();

            }

            response.insert("Access token", accessToken);
            response.insert("Refresh token", refreshToken);
            response.insert("Nickname", nickname);

            query.prepare("SELECT LastBan FROM users WHERE Nickname = ?");
            query.addBindValue(nickname);
            query.exec();

            while(query.next())
                banFinish = query.value(0).toInt();

            if(QDateTime::currentDateTime().toTime_t() < banFinish)
                response.insert("Ban", int(banFinish));
        }
        else
            response.insert("Value", "Authorization failed");
    }

    return response;
}

QJsonObject Connection::registration(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Registration");

    QSqlQuery query;
    query.prepare("SELECT ID FROM users WHERE Email=? OR Nickname=?");
    query.bindValue(0, request.value("Email").toString());
    query.bindValue(1, request.value("Nickname").toString());
    query.exec();

    QString id="";
    while (query.next())
        id = query.value(0).toString();

    if(id==""){
        query.prepare("INSERT INTO registrationQueue(Email, Code) VALUES (?, ?)");
        query.bindValue(0, request.value("Email").toString());
        QString code = QString().append(QCryptographicHash::hash(QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentDateTime().toTime_t()), QCryptographicHash::Md5).toHex()).left(6);
        query.bindValue(1, code);
        query.exec();

        Smtp *smtp = new Smtp(EMAIL_USER, EMAIL_PASSWORD, "smtp.gmail.com");
        smtp->sendMail(EMAIL_USER, request.value("Email").toString(), EMAIL_REGISTRATION_SUBJECT,
                       EMAIL_REGISTRATION_BODY.arg(request.value("Nickname").toString(), code));

        response.insert("Value", "Email doesn't exist");
    }
    else
        response.insert("Value", "Email exists");

    return response;
}

QJsonObject Connection::registrationCode(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Registration code");

    QSqlQuery query;

    QString code = "";
    query.prepare("SELECT Code FROM registrationQueue WHERE Email = ?");
    query.bindValue(0, request.value("Email").toString());
    query.exec();

    while(query.next())
        code = query.value(0).toString();

    if(code == request.value("Code").toString()){
        query.prepare("DELETE FROM registrationQueue WHERE Email = ?");
        query.bindValue(0, request.value("Email").toString());
        query.exec();

        uint registrationTime = QDateTime::currentDateTime().toTime_t();
        QPair<Salt, Password> pair = hashPassword(request.value("Password").toString(), registrationTime);

        query.prepare("INSERT INTO users (Email, Nickname, Password, Date, Salt) VALUES (?, ?, ?, ?, ?)");
        query.bindValue(0, request.value("Email").toString());
        query.bindValue(1, request.value("Nickname").toString());
        query.bindValue(2, pair.second);
        query.bindValue(3, registrationTime);
        query.bindValue(4, pair.first);
        query.exec();

        response.insert("Value", "Registration successful");
    }
    else
        response.insert("Value", "Invalid code");

    return response;
}

QJsonObject Connection::recovery(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Recovery");

    QSqlQuery query;
    query.prepare("SELECT Email, Nickname FROM users WHERE Email=? OR Nickname=?");
    query.bindValue(0, request.value("Value"));
    query.bindValue(1, request.value("Value"));
    query.exec();

    QString email="";
    QString nickname="";
    while (query.next()){
        email = query.value(0).toString();
        nickname = query.value(1).toString();
    }
    if(email!=""){
        QString code = QString().append(QCryptographicHash::hash(QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentDateTime().toTime_t()), QCryptographicHash::Md5).toHex()).left(6);
        Smtp *smtp = new Smtp(EMAIL_USER, EMAIL_PASSWORD, "smtp.gmail.com");
        smtp->sendMail(EMAIL_USER, email, EMAIL_RECOVERY_SUBJECT,
                       EMAIL_RECOVERY_BODY.arg(nickname, code));

        query.prepare("INSERT INTO recoveryQueue (Email, Code) VALUES(?, ?)");
        query.bindValue(0, email);
        query.bindValue(1, code);
        query.exec();

        response.insert("Value", "Founded");
    }
    else
        response.insert("Value", "Not founded");

    return response;
}

QJsonObject Connection::recoveryCode(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Recovery code");

    QSqlQuery query;
    query.prepare("SELECT Email FROM users WHERE Email=? OR Nickname=?");
    query.bindValue(0, request.value("Value").toString());
    query.bindValue(1, request.value("Value").toString());
    query.exec();

    QString email="";
    while (query.next())
        email = query.value(0).toString();

    QString code = "";
    query.prepare("SELECT Code FROM recoveryQueue WHERE Email = ?");
    query.addBindValue(email);
    query.exec();

    while(query.next())
        code = query.value(0).toString();

    if(request.value("Code") == code){
        query.prepare("UPDATE recoveryQueue SET Code = 'Confirmed' WHERE Email = ?");
        query.addBindValue(email);
        query.exec();

        response.insert("Value", "Right code");
    }
    else
        response.insert("Value", "Invalid code");

    return response;
}

QJsonObject Connection::recoveryNewPass(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Recovery new pass");

    QSqlQuery query;
    query.prepare("SELECT Email, Nickname FROM users WHERE Email=? OR Nickname=?");
    query.bindValue(0, request.value("Value").toString());
    query.bindValue(1, request.value("Value").toString());
    query.exec();

    QString email="";
    QString nickname="";
    while (query.next()){
        email = query.value(0).toString();
        nickname = query.value(1).toString();
    }

    query.prepare("SELECT ID FROM recoveryQueue WHERE Email = ? AND Code = 'Confirmed'");
    query.bindValue(0, email);

    query.exec();

    QString ID = "";
    while(query.next())
        ID = query.value(0).toString();

    if(ID != ""){
        query.prepare("DELETE FROM recoveryQueue WHERE Email = ?");
        query.bindValue(0, email);
        query.exec();

        query.prepare("SELECT Date FROM users WHERE Email = ?");
        query.addBindValue(email);
        query.exec();

        int date;
        while(query.next())
            date = query.value(0).toInt();

        QPair<Salt, Password> pair = hashPassword(request.value("Password").toString(), date);
        query.prepare("UPDATE users SET Password=?, Salt=? WHERE Email=?");
        query.bindValue(0, pair.second);
        query.bindValue(1, pair.first);
        query.bindValue(2, email);
        query.exec();

        response.insert("Value", "Password changed successfully");
        emit passwordChanged(email, nickname);
    }
    else
        response.insert("Value", "Password hasn't been changed");

    return response;
}

QJsonObject Connection::doesNicknameExist(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "DoesNicknameExist");

    QSqlQuery query;
    query.prepare("SELECT ID FROM users WHERE Nickname=?");
    query.bindValue(0, request.value("Nickname").toString());
    query.exec();

    QString id="";
    while (query.next())
        id = query.value(0).toString();

    response.insert("Value", id != "" ? "Nickname exists" : "Nickname doesn't exist");
    return response;
}

QJsonObject Connection::bansHistory(int page){
    QJsonObject response;
    response.insert("Target", "Bans history");

    QSqlQuery query;

    static int lastBan;

    if(!page){
        query.prepare("SELECT MAX(ID) FROM bans");
        query.exec();

        while (query.next())
            lastBan = query.value(0).toInt();
    }

    query.prepare("SELECT Nickname, Cause, Moderator, StartTime, FinishTime FROM bans WHERE ID >= ? AND ID <= ?");
    query.addBindValue(lastBan-29*(page+1) < 1 ? 1 : lastBan-29*(page+1));
    query.addBindValue(lastBan-29*page);
    query.exec();

    QJsonArray array;
    while(query.next()){
        QJsonObject obj;
        obj.insert("Nickname", query.value(0).toString());
        obj.insert("Cause", query.value(1).toString());
        obj.insert("Moderator", query.value(2).toString());
        obj.insert("StartTime", query.value(3).toInt());
        obj.insert("FinishTime", query.value(4).toInt());
        array.append(obj);
    }

    response.insert("Page", array);
    if(29*(page+1) > MAXIMUM_NUM_OF_BANS_TO_SHOW || 29*(page+1) >= lastBan)
        response.insert("Value", "End");

    return response;
}

void Connection::accessTokenRefreshing(){
    accessToken = QString().append(QCryptographicHash::hash(nickname.toUtf8() + QByteArray::number(qrand()) + QByteArray::number(QDateTime::currentDateTime().toTime_t()), QCryptographicHash::Md5).toHex());
    tokenTime = QDateTime::currentDateTime().toTime_t() + 60*60*24;

    QSqlQuery query;
    query.prepare("UPDATE users SET AccessToken = ?, TokenTime = ? WHERE Nickname = ?");
    query.bindValue(0, accessToken);
    query.bindValue(1, tokenTime);
    query.bindValue(2, nickname);
    query.exec();
}

void Connection::sendGlobalMessage(QJsonObject request){
    if(location != "GlobalChat" || QDateTime::currentDateTime().toTime_t() < banFinish)
        return;

    QJsonObject response;
    response.insert("Target", "Message status");

    QString text = request.value("Message").toString();

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
            floodTimer = QDateTime::currentDateTime().toTime_t()+3*floodCounter;
            response.insert("Value", "Flood");
            response.insert("Time", 3000*floodCounter);
            isMessageGoingToBeSended = false;
        }
        else if(count>=3){
            QSqlQuery query;
            query.prepare("UPDATE users SET LastBan = ? WHERE Nickname = ?");
            query.bindValue(0, QDateTime::currentDateTime().toTime_t()+14400); //4 hours for flood
            query.bindValue(1, nickname);
            if(!query.exec())
                qDebug() << query.lastError().text();

            //for bans history
            banFinish = QDateTime::currentDateTime().toTime_t()+14400;
            query.prepare("INSERT INTO bans (Nickname, Message, StartTime, FinishTime, Cause, Moderator) VALUES (?, ?, ?, ?, 'Flood', 'System')");
            query.bindValue(0, nickname);
            query.bindValue(1, text);
            query.bindValue(2, QDateTime::currentDateTime().toTime_t());
            query.bindValue(3, banFinish);
            if(!query.exec())
                qDebug() << query.lastError().text();

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

        if(request.contains("Attachment") && request.value("Attachment").toString().length() <= 100){
            query.prepare("INSERT INTO messages (Sender, Text, Time, Attachment) VALUES (?, ?, ?, ?)");
            query.bindValue(3, request.value("Attachment").toString());
        }
        else
            query.prepare("INSERT INTO messages (Sender, Text, Time) VALUES (?, ?, ?)");

        query.bindValue(0, nickname);
        query.bindValue(1, text);
        query.bindValue(2, QDateTime::currentDateTime().toTime_t());
        query.exec();
        socket->write(QJsonDocument(response).toJson());
        socket->waitForBytesWritten();
        emit dispatchMessage();
        return;
    }
    socket->write(QJsonDocument(response).toJson());
}

QJsonObject Connection::banFinished(){
    QJsonObject response;
    response.insert("Target", "Ban finished");

    QSqlQuery query;
    query.prepare("SELECT LastBan FROM users WHERE Nickname = ?");
    query.addBindValue(nickname);
    query.exec();

    uint time;
    while(query.next())
        time = query.value(0).toInt();

    response.insert("Value", QDateTime::currentDateTime().toTime_t() <= time ? "True" : "False");

    return response;
}

QJsonObject Connection::exit(){
    nickname = "";
    location = "";
    accessToken = "";
    refreshToken = "";
    tokenTime = -1;

    QJsonObject response;
    response.insert("Target", "Exit");

    return response;
}

void Connection::disconnecting(){
    emit disconnected(socket->socketDescriptor());
}

void Connection::controller(){
    QByteArray receivedObject = socket->readAll();
    QJsonObject response;

    QJsonParseError error;

    QJsonObject request = QJsonDocument::fromJson(receivedObject, &error).object();

    if(error.error == QJsonParseError::NoError){
        if(request.value("Target").toString() == "Authorization")
            response = authorization(request);
        else if(request.value("Target").toString() == "Registration")
            response = registration(request);
        else if(request.value("Target").toString() == "Registration code")
            response = registrationCode(request);
        else if(request.value("Target").toString() == "Recovery")
            response = recovery(request);
        else if(request.value("Target").toString() == "Recovery code")
            response = recoveryCode(request);
        else if(request.value("Target").toString() == "Recovery new pass")
            response = recoveryNewPass(request);
        else if(request.value("Target").toString() == "DoesNicknameExist")
            response = doesNicknameExist(request);
        //ACCESS TOKEN CHECKING
        else if(request.value("Target").toString() == "Token refreshing"){
            QSqlQuery query;
            query.prepare("SELECT AccessToken, TokenTime FROM users WHERE Nickname = ? AND RefreshToken = ?");
            query.bindValue(0, nickname);
            query.bindValue(1, request.value("Refresh token").toString());
            query.exec();

            QString accessToken = "";
            while(query.next()){
                accessToken = query.value(0).toString();
                tokenTime = query.value(1).toInt();
            }

            if(accessToken == "")
                response.insert("Target", "Exit");
            else{
                if(tokenTime <= int(QDateTime::currentDateTime().toTime_t()))
                    accessTokenRefreshing();
                else
                    this->accessToken = accessToken;

                response.insert("Target", "Token refreshed");
                response.insert("Access token", this->accessToken);
                response.insert("Nickname", nickname);
            }
        }
        else{
            QSqlQuery query;
            query.prepare("SELECT RefreshToken, TokenTime FROM users WHERE Nickname = ? AND AccessToken = ?");
            query.addBindValue(nickname);
            query.addBindValue(accessToken);
            query.exec();

            QString refreshToken = "";
            while (query.next()){
                refreshToken = query.value(0).toString();
                tokenTime = query.value(1).toInt();
            }

            if(tokenTime <= int(QDateTime::currentDateTime().toTime_t()) || refreshToken == "")
                response.insert("Target", "Token refreshing");
            else{
                if(request.value("Target").toString() == "PMessage"){
                    //TODO
                }
                else if(request.value("Target").toString() == "GMessage"){
                    if(int(QDateTime::currentDateTime().toTime_t()) < floodTimer)
                        return;
                    sendGlobalMessage(request);
                    return;
                }
                else if(request.value("Target").toString() == "Post"){
                    //TODO
                }
                else if(request.value("Target").toString() == "Ban finished")
                    response = banFinished();
                else if(request.value("Target").toString() == "Exit")
                    response = exit();
                else if(request.value("Target").toString() == "Bans history")
                    response = bansHistory(request.value("Page").toInt());
                else if(request.value("Target").toString() == "Location"){
                    location = request.value("Value").toString() < 20 ? request.value("Value").toString() : request.value("Value").toString().left(20);
                    return;
                }
            }
        }
    }
    socket->write(QJsonDocument(response).toJson());
}

void Connection::sendEmailPasswordChanged(QString email, QString nickname){
    Smtp *smtp = new Smtp(EMAIL_USER, EMAIL_PASSWORD, "smtp.gmail.com");
    smtp->sendMail(EMAIL_USER, email, EMAIL_PASSWORD_CHANGED_SUBJECT,
                   EMAIL_PASSWORD_CHANGED_BODY.arg(nickname));
}
