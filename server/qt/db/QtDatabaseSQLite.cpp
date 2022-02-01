#include "QtDatabaseSQLite.hpp"

#include <iostream>

using namespace CatchChallenger;

QtDatabaseSQLite::QtDatabaseSQLite()
{
    setObjectName("QtDatabaseSQLite");
}

QtDatabaseSQLite::~QtDatabaseSQLite()
{
}

bool QtDatabaseSQLite::syncConnect(const std::string &host, const std::string &dbname, const std::string &user,const std::string &password)
{
    const std::string &file=host;
    (void)dbname;
    (void)user;
    (void)password;
    if(conn!=NULL)
        syncDisconnect();
    conn=findConnexionToOpen(file,std::string(),DatabaseBase::DatabaseType::SQLite);
    if(conn!=NULL)
    {
        databaseTypeVar=DatabaseBase::DatabaseType::SQLite;
        return true;
    }
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QSQLITE","server"+QString::number(QtDatabase::establishedConnexionCount));
    conn->setDatabaseName(file.c_str());
    if(!conn->open())
    {
        lastErrorMessage=(conn->lastError().driverText()+": "+conn->lastError().databaseText()).toStdString();
        delete conn;
        conn=NULL;
        return false;
    }
    databaseTypeVar=DatabaseBase::DatabaseType::SQLite;

    QtDatabase::EstablishedConnexion establishedConnexion;
    establishedConnexion.conn=conn;
    establishedConnexion.host=file;
    establishedConnexion.openCount=1;
    establishedConnexion.type=DatabaseBase::DatabaseType::SQLite;
    QtDatabase::establishedConnexionList.push_back(establishedConnexion);
    QtDatabase::establishedConnexionCount++;

    return true;
}
