#include "server.h"

Server::Server(QObject *parent) : QTcpServer(parent){
    ftpSocket = new QTcpSocket(this);
    ftpSocket->connectToHost(QHostAddress::LocalHost, FTP_PORT);

    connections.clear();

    messageSender.setConnections(&connections);

    listen(QHostAddress::Any, PORT);

    connect(ftpSocket, SIGNAL(readyRead()), SLOT(ftpController()));
}


void Server::incomingConnection(qintptr handle){
    connections.insert(handle, new Connection(handle));
    connect(connections.value(handle), SIGNAL(disconnected(qintptr)), SLOT(deleteConnection(qintptr)));
    connect(connections.value(handle), SIGNAL(dispatchMessage()), SLOT(dispatchingMessage()));
}

void Server::deleteConnection(qintptr key){
    connections.erase(connections.find(key));
}

void Server::dispatchingMessage(){
    if(!messageSender.isRunning)
        messageSender.start();
}

void Server::ftpController(){
    QByteArray receivedObject = ftpSocket->readAll();

    QJsonObject response;
    QJsonObject request = QJsonDocument::fromJson(receivedObject).object();

    if(request.value("Target").toString() == "Post"){
        response.insert("Target", "Post");
        response.insert("Socket handle", request.value("Socket handle").toInt());
        response.insert("Location", request.value("Location").toString());

        QSqlQuery query;
        int ban = -1;
        int id = -1;
        query.prepare("SELECT LastBan, ID FROM users WHERE AccessToken = ? AND Nickname = ?");
        query.bindValue(0, request.value("Access token").toString());
        query.bindValue(1, request.value("Nickname").toString());
        query.exec();

        while(query.next()){
            ban = query.value(0).toInt();
            id = query.value(1).toInt();
        }

        if((ban == -1 && id == -1) || ban >= int(QDateTime::currentDateTime().toTime_t()))
            response.insert("Value", "Deny");
        else if(request.value("Location").toString() == "GlobalChat"){
            response.insert("ID", id);
            response.insert("Value", "Allow");
        }
        /*
        else if(request.value("Location").toString() == "Avatar"){
            response.insert("ID", id);
            response.insert("Value", "Allow");
            query.prepare("UPDATE users SET Avatar=? WHERE ID = ?");
        }
        */
    }
    else if(request.value("Target").toString() == "Get"){
        response.insert("Target", "Get");
        response.insert("Socket handle", request.value("Socket handle").toInt());
        response.insert("Reference",  request.value("Reference").toString());

        QSqlQuery query;
        int id = -1;
        query.prepare("SELECT ID FROM users WHERE AccessToken = ? AND Nickname = ?");
        query.bindValue(0, request.value("Access token").toString());
        query.bindValue(1, request.value("Nickname").toString());
        query.exec();

        while(query.next())
            id = query.value(0).toInt();

        if(id == -1)
            response.insert("Value", "Deny");
        else
            response.insert("Value", "Allow");
    }

    ftpSocket->write(QJsonDocument(response).toJson());
}








