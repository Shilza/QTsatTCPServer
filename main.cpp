#include <QCoreApplication>
#include <QSqlDatabase>
#include "messagesender.h"
#include "def.h"
#include "Smtp.h"


int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("Tsat");
    db.setUserName("root");
    //db.setUserName(DATABASE_USER);
    //db.setPassword(DATABASE_PASSWORD);
    if(!db.open())
        return 1;

    Smtp *smtp = new Smtp(" ", " ", "smtp.gmail.com");

    smtp->sendMail("anatolich1995@gmail.com", "evertonnaxoi@gmail.com" , "This is a subject", "This is a body");

//    MessageSender server;

    return a.exec();
}
