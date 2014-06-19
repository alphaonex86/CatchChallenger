#include "QtDatabase.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <QDebug>
#include <QSqlError>

using namespace CatchChallenger;

char QtDatabase::emptyString[]={'\0'};

QtDatabase::QtDatabase() :
    conn(NULL),
    sqlQuery(NULL)
{
}

QtDatabase::~QtDatabase()
{
    if(sqlQuery!=NULL)
        delete sqlQuery;
    if(conn!=NULL)
        delete conn;
}

bool QtDatabase::isConnected() const
{
    return conn!=NULL;
}

bool QtDatabase::syncConnect(const char * host, const char * dbname, const char * user, const char * password)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QMYSQL","server");
    conn->setConnectOptions("MYSQL_OPT_RECONNECT=1");
    conn->setHostName(host);
    conn->setDatabaseName(dbname);
    conn->setUserName(user);
    conn->setPassword(password);
    if(!conn->open())
    {
        delete conn;
        conn=NULL;
        return false;
    }
    if(conn!=NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    return true;
}

bool QtDatabase::syncConnectMysql(const char * host, const char * dbname, const char * user, const char * password)
{
    return syncConnect(host,dbname,user,password);
}

bool QtDatabase::syncConnectSqlite(const char * file)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QSQLITE","server");
    conn->setDatabaseName(file);
    if(!conn->open())
    {
        delete conn;
        conn=NULL;
        return false;
    }
    if(conn!=NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    return true;
}

bool QtDatabase::syncConnectPostgresql(const char * host, const char * dbname, const char * user, const char * password)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QPSQL","server");
    conn->setHostName(host);
    conn->setDatabaseName(dbname);
    conn->setUserName(user);
    conn->setPassword(password);
    if(!conn->open())
    {
        delete conn;
        conn=NULL;
        return false;
    }
    if(conn!=NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    return true;
}

void QtDatabase::syncDisconnect()
{
    if(conn==NULL)
    {
        std::cerr << "db not connected" << std::endl;
        return;
    }
    conn->close();
    delete conn;
    conn=NULL;
}

bool QtDatabase::asyncRead(const char *query,void * returnObject, CallBackDatabase method)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    if(query!=NULL)
        delete query;
    sqlQuery=new QSqlQuery(*conn);
    if(!sqlQuery->exec(query))
    {
        qDebug() << QString(sqlQuery->lastQuery()+": "+sqlQuery->lastError().text());
        return false;
    }
    method(returnObject);
    delete query;
    query=NULL;
    return true;
}

bool QtDatabase::asyncWrite(const char *query)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    QSqlQuery writeQuery(*conn);
    if(!writeQuery.exec(query))
    {
        qDebug() << QString(writeQuery.lastQuery()+": "+writeQuery.lastError().text());
        return false;
    }
    return true;
}

void QtDatabase::clear()
{
    if(sqlQuery!=NULL)
    {
        delete sqlQuery;
        sqlQuery=NULL;
    }
}

char *QtDatabase::errorMessage()
{
    return (conn->lastError().driverText()+QString(": ")+conn->lastError().databaseText()).toUtf8().data();
}

bool QtDatabase::next()
{
    if(conn==NULL)
        return false;
    if(sqlQuery==NULL)
        return false;
    if(!sqlQuery->next())
    {
        delete sqlQuery;
        sqlQuery=NULL;
    }
    return true;
}

char * QtDatabase::value(const int &value)
{
    if(sqlQuery==NULL)
        return emptyString;
    return sqlQuery->value(value).toString().toUtf8().data();
}
