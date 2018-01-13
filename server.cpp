#include "server.h"

Server::Server(QObject *parent) : QTcpServer(parent){
    connections.clear();

    messageSender.setConnections(&connections);
    //messageSender.moveToThread(&senderThread);

    listen(QHostAddress::Any, PORT);

    //connect(&senderThread, SIGNAL(started()), &messageSender, SLOT(start()));
    //connect(&messageSender, SIGNAL(finished()), &senderThread, SLOT(quit()));
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
