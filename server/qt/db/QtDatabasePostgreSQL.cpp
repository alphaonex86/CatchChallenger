#include "QtDatabasePostgreSQL.hpp"

#include <iostream>

using namespace CatchChallenger;

QtDatabasePostgreSQL::QtDatabasePostgreSQL()
{
    setObjectName("QtDatabasePostgreSQL");
}

QtDatabasePostgreSQL::~QtDatabasePostgreSQL()
{
}

bool QtDatabasePostgreSQL::syncConnect(const std::string &host,const std::string &dbname,const std::string &user,const std::string &password)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    std::cerr << "Disconnected to incorrect database type for solo into QtDatabase class" << std::endl;
    abort();
    #endif
    #endif
    if(conn!=NULL)
        syncDisconnect();
    conn=findConnexionToOpen(host,dbname,DatabaseBase::DatabaseType::PostgreSQL);
    if(conn!=NULL)
    {
        databaseTypeVar=DatabaseBase::DatabaseType::PostgreSQL;
        return true;
    }
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QPSQL","server"+QString::number(QtDatabase::establishedConnexionCount));
    std::string tempString(host);
    if(tempString!="localhost")
        conn->setHostName(host.c_str());
    conn->setDatabaseName(dbname.c_str());
    conn->setUserName(user.c_str());
    conn->setPassword(password.c_str());
    if(!conn->open())
    {
        lastErrorMessage=(conn->lastError().driverText()+": "+conn->lastError().databaseText()).toStdString();
        delete conn;
        conn=NULL;
        return false;
    }
    databaseTypeVar=DatabaseBase::DatabaseType::PostgreSQL;

    QtDatabase::EstablishedConnexion establishedConnexion;
    establishedConnexion.conn=conn;
    establishedConnexion.dbname=dbname;
    establishedConnexion.host=host;
    establishedConnexion.openCount=1;
    establishedConnexion.type=DatabaseBase::DatabaseType::PostgreSQL;
    QtDatabase::establishedConnexionList.push_back(establishedConnexion);
    QtDatabase::establishedConnexionCount++;

    return true;
}
