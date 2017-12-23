#include "server.h"

Server::Server(QObject *parent) : QTcpServer(parent){
    connections.clear();


    this->listen(QHostAddress::Any, 40000);
}

QHash<qintptr, User *> *Server::getConnections(){
    return &connections;
}

void Server::incomingConnection(qintptr handle){
    connections.insert(handle, new User(handle));
    connect(connections.value(handle), SIGNAL(disconnected(qintptr)), SLOT(deleteConnection(qintptr)));
    connect(connections.value(handle), SIGNAL(dispatchMessage()), SLOT(dispatchingMessage()));
    connect(connections.value(handle), SIGNAL(newRecovery(QString,QString)), SLOT(newRecovery(QString,QString)));
    connect(connections.value(handle), SIGNAL(newRegistration(QString,QString)), SLOT(newRegistration(QString,QString)));
    connect(connections.value(handle), SIGNAL(registrationCode(QJsonObject)), SLOT(registrationCode(QJsonObject)));
    connect(connections.value(handle), SIGNAL(recoveryCode(QString,QString)), SLOT(recoveryCode(QString, QString)));
    connect(connections.value(handle), SIGNAL(recoveryNewPass(QString,QString)), SLOT(recoveryNewPass(QString,QString)));

    QTcpServer::incomingConnection(handle);
}

void Server::deleteConnection(qintptr key){
    connections.erase(connections.find(key));
}

void Server::dispatchingMessage(){
    emit startDispatching();
}

void Server::newRegistration(QString email, QString code){
    registrationList.insert(email, code);
}

void Server::registrationCode(QJsonObject request){
    QJsonObject response;
    response.insert("Target", "Registraion code");
    if(registrationList.value(request.value("Email").toString()) == request.value("Code").toString()){
        registrationList.erase(registrationList.find(request.value("Email").toString()));
        QSqlQuery query;
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

    ((User*)sender())->getSocket()->write(QJsonDocument(response).toJson());
}

void Server::newRecovery(QString email, QString code){
    recoveryList.insert(email, code);
}

void Server::recoveryCode(QString email, QString code){
    QJsonObject response;
    response.insert("Target", "Recovery code");

    if(recoveryList.value(email) == code){
        recoveryList.insert(email, EMAIL_IS_CONFIRMED);
        response.insert("Value", "Right code");
    }
    else
        response.insert("Value", "Invalid code");

    ((User*)sender())->getSocket()->write(QJsonDocument(response).toJson());
}

void Server::recoveryNewPass(QString email, QString pass){
    QJsonObject response;
    response.insert("Target", "Recovery new pass");

    if(recoveryList.value(email) == EMAIL_IS_CONFIRMED){
        recoveryList.erase(recoveryList.find(email));
        QSqlQuery query;
        query.prepare("UPDATE users SET Password=? WHERE Email=? VALUES (?, ?)");
        query.bindValue(0, email);
        query.bindValue(1, pass);
        query.exec();
        response.insert("Value", "Password changed successfully");
    }
    else
        response.insert("Value", "Password hasn't been changed");

    ((User*)sender())->getSocket()->write(QJsonDocument(response).toJson());
}

