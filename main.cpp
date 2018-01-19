#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include "server.h"
#include "def.h"

int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("Tsat");
    db.setUserName("root");
    //db.setUserName(DATABASE_USER);
    //db.setPassword(DATABASE_PASSWORD);
    db.setConnectOptions("MYSQL_OPT_RECONNECT=1");
    if(!db.open())
        return 1;

    qsrand(QTime(0,0,0).msecsTo(QTime::currentTime()));

/*
    QSqlQuery query;
    for(int i=0; i<100; i++)
        query.exec("INSERT INTO bans(Nickname, Cause, Moderator, StartTime, FinishTime, Message) VALUES ('Shilza', 'Flood', 'System', 1514643906, 1514658306, 'AAAAA')");
*/
    Server server;

    return a.exec();
}
