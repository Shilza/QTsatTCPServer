#include "messagesender.h"

MessageSender::MessageSender(){

    QSqlQuery query;
    query.prepare("SELECT MAX(id) FROM messages");
    query.exec();

    while (query.next())
        lastMessage = query.value(0).toInt();

    connect(&server, SIGNAL(startDispatching()), SLOT(start()));
}

void MessageSender::start(){
    if(!isRunning())
        run();
}

void MessageSender::run(){
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
        for(User* a : *server.getConnections())
            a->send(document);
    }
}
