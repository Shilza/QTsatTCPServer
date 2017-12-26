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
    QSqlQuery query;
    while(true){
        query.prepare("SELECT Sender, Text, Time FROM messages WHERE id = ?");
        query.bindValue(0, ++lastMessage);
        if(!query.exec()){
            lastMessage--;
            break;
        }

        QJsonObject result;
        while (query.next()){
            result.insert("Target", "Message delivery");
            result.insert("Nickname", query.value(0).toString());
            result.insert("Message", query.value(1).toString());
            result.insert("Message", query.value(2).toInt());
        }

        QJsonDocument document(result);
        for(Connection* a : *connections)
            a->send(document);
    }
    emit finished();
}
