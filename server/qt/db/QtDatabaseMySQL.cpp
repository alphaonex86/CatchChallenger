#include "QtDatabaseMySQL.hpp"

#include <iostream>

using namespace CatchChallenger;


QtDatabaseMySQL::QtDatabaseMySQL()
{
    setObjectName("QtDatabaseMySQL");
}

QtDatabaseMySQL::~QtDatabaseMySQL()
{
}

bool QtDatabaseMySQL::syncConnect(const std::string &host, const std::string &dbname, const std::string &user,const std::string &password)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    std::cerr << "Disconnected to incorrect database type for solo into QtDatabase class" << std::endl;
    abort();
    #endif
    #endif
    if(conn!=NULL)
        syncDisconnect();
    conn=findConnexionToOpen(host,dbname,DatabaseBase::DatabaseType::Mysql);
    if(conn!=NULL)
    {
        databaseTypeVar=DatabaseBase::DatabaseType::Mysql;
        return true;
    }
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QMYSQL","server"+QString::number(QtDatabase::establishedConnexionCount));
    conn->setConnectOptions("MYSQL_OPT_RECONNECT=1");
    conn->setHostName(host.c_str());
    conn->setDatabaseName(dbname.c_str());
    conn->setUserName(user.c_str());
    conn->setPassword(password.c_str());
    if(!conn->open())
    {
        delete conn;
        conn=NULL;
        return false;
    }
    databaseTypeVar=DatabaseBase::DatabaseType::Mysql;

    QtDatabase::EstablishedConnexion establishedConnexion;
    establishedConnexion.conn=conn;
    establishedConnexion.dbname=dbname;
    establishedConnexion.host=host;
    establishedConnexion.openCount=1;
    establishedConnexion.type=DatabaseBase::DatabaseType::Mysql;
    QtDatabase::establishedConnexionList.push_back(establishedConnexion);
    QtDatabase::establishedConnexionCount++;

    return true;
}
