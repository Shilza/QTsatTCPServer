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

    Server server;

    return a.exec();
}
