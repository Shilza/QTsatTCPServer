#include "connection.h"
#include <QSqlError>


Connection::Connection(qintptr handle, QObject *parent) : QObject(parent){
    socket = new QTcpSocket(this);
    socket->setSocketDescriptor(handle);
    location = "";

    lastMessages.clear();

    connect(socket, SIGNAL(disconnected()), SLOT(disconnecting()));
    connect(socket, SIGNAL(readyRead()), SLOT(controller()));
    connect(this, SIGNAL(passwordChanged(QString, QString)), SLOT(sendEmailPasswordChanged(QString, QString)));
}

void Connection::send(QJsonDocument message){
    socket->write(message.toJson());
}

QJsonObject Connection::authorization(QJsonObject request){
    QJsonObject response;
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

        query.prepare("INSERT INTO users (Email, Nickname, Password, Date) VALUES (:email, :nickname, :password, :date)");
        query.bindValue(":email", request.value("Email").toString());
        query.bindValue(":nickname", request.value("Nickname").toString());
        query.bindValue(":password", request.value("Password").toString());
        query.bindValue(":date", QDateTime::currentDateTime().toTime_t());
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
    query.bindValue(0, email);
    query.exec();

    while(query.next())
        code = query.value(0).toString();

    if(request.value("Code") == code){
        query.prepare("UPDATE recoveryQueue SET Code = 'Confirmed'");
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

        query.prepare("UPDATE users SET Password=? WHERE Email=?");
        query.bindValue(0, request.value("Password"));
        query.bindValue(1, email);
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

QJsonObject Connection::sendGlobalMessage(QJsonObject request){
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
        if(query.exec()){
            qDebug() << "sfef";
            emit dispatchMessage();
        }
        else
            qDebug() << query.lastError().text();
    }
    return response;
}

void Connection::disconnecting(){
    emit disconnected(socket->socketDescriptor());
    socket->deleteLater();
}

void Connection::controller(){
    QByteArray receivedObject = socket->readAll();
    QJsonParseError error;

    QJsonObject request = QJsonDocument::fromJson(receivedObject, &error).object();
    QJsonObject response;
    if(error.error == QJsonParseError::NoError){
        if(request.value("Target").toString() == "Authorization"){
            response = authorization(request);
        }
        else if(request.value("Target").toString() == "Registration"){
            response = registration(request);
        }
        else if(request.value("Target").toString() == "Registration code"){
            response = registrationCode(request);
        }
        else if(request.value("Target").toString() == "Recovery"){
            response = recovery(request);
        }
        else if(request.value("Target").toString() == "Recovery code"){
            response = recoveryCode(request);
        }
        else if(request.value("Target").toString() == "Recovery new pass"){
            response = recoveryNewPass(request);
        }
        else if(request.value("Target").toString() == "PMessage"){
            //TODO
        }
        else if(request.value("Target").toString() == "GMessage"){
            if(QDateTime::currentDateTime().toTime_t() < floodTimer)
                return;

            response = sendGlobalMessage(request);
        }
        else if(request.value("Target").toString() == "Post"){

        }
        else if(request.value("Target").toString() == "DoesNicknameExist"){
            response = doesNicknameExist(request);
        }
    }
    socket->write(QJsonDocument(response).toJson());
}

void Connection::sendEmailPasswordChanged(QString email, QString nickname){
    Smtp *smtp = new Smtp(EMAIL_USER, EMAIL_PASSWORD, "smtp.gmail.com");
    smtp->sendMail(EMAIL_USER, email, EMAIL_PASSWORD_CHANGED_SUBJECT,
                   EMAIL_PASSWORD_CHANGED_BODY.arg(nickname));
}
