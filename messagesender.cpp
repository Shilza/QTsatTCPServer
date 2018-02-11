#include "messagesender.h"

MessageSender::MessageSender(){

    QSqlQuery query;
    query.prepare("SELECT MAX(id) FROM messages");
    query.exec();

    while (query.next())
        lastMessage = query.value(0).toInt();
}

void MessageSender::setConnections(QHash<qintptr, Connection *> *connections){
    this->connections = connections;
}

void MessageSender::start(){
    isRunning = true;
    QSqlQuery query;
    while(true){
        query.prepare("SELECT Sender, Text, Time FROM messages WHERE id = ?");
        query.bindValue(0, ++lastMessage);
        query.exec();

        QJsonObject result;
        bool temp = false;
        while (query.next()){
            temp = true;
            result.insert("Target", "Message delivery");
            result.insert("Nickname", query.value(0).toString());
            result.insert("Message", query.value(1).toString());
            result.insert("Time", query.value(2).toInt());
        }

        if(!temp){
            lastMessage--;
            break;
        }

        QJsonDocument document(result);
        for(Connection* a : *connections){
            if (a->getLocation() == "GlobalChat")
                a->send(document);
        }
    }
    isRunning = false;
}
