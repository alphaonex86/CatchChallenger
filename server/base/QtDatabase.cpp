#include "QtDatabase.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <QSqlError>
#include <QDebug>

using namespace CatchChallenger;

char QtDatabase::emptyString[]={'\0'};

QtDatabase::QtDatabase() :
    conn(NULL),
    sqlQuery(NULL)
{
    connect(this,&QtDatabase::sendQuery,&dbThread,&QtDatabaseThread::receiveQuery,Qt::QueuedConnection);
    connect(&dbThread,&QtDatabaseThread::sendReply,this,&QtDatabase::receiveReply,Qt::QueuedConnection);
    databaseConnected=DatabaseBase::DatabaseType::Mysql;
}

QtDatabase::~QtDatabase()
{
    if(sqlQuery!=NULL)
        delete sqlQuery;
    if(conn!=NULL)
    {
        conn->close();
        delete conn;
        conn=NULL;
    }
}

bool QtDatabase::isConnected() const
{
    return conn!=NULL;
}

bool QtDatabase::syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QMYSQL","server");
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
    databaseConnected=DatabaseBase::DatabaseType::Mysql;
    return true;
}

bool QtDatabase::syncConnectMysql(const std::string &host, const std::string &dbname, const std::string &user,const std::string &password)
{
    return syncConnect(host,dbname,user,password);
}

bool QtDatabase::syncConnectSqlite(const std::string &file)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QSQLITE","server");
    conn->setDatabaseName(file.c_str());
    if(!conn->open())
    {
        lastErrorMessage=(conn->lastError().driverText()+": "+conn->lastError().databaseText()).toStdString();
        delete conn;
        conn=NULL;
        return false;
    }
    databaseConnected=DatabaseBase::DatabaseType::SQLite;
    return true;
}

bool QtDatabase::syncConnectPostgresql(const std::string &host,const std::string &dbname,const std::string &user,const std::string &password)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QPSQL","server");
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
    databaseConnected=DatabaseBase::DatabaseType::PostgreSQL;
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
    databaseConnected=DatabaseBase::DatabaseType::Mysql;
}

DatabaseBase::CallBack *QtDatabase::asyncRead(const std::string &query,void * returnObject, CallBackDatabase method)
{
    if(conn==NULL)
    {
        std::cerr << "db not connected" << std::endl;
        return NULL;
    }
    CallBack tempCallback;
    tempCallback.object=returnObject;
    tempCallback.method=method;
    if(queue.size()>0)
    {
        if(queue.size()>=CATCHCHALLENGER_MAXBDQUERIES)
        {
            std::cerr << "db queue full" << std::endl;
            return NULL;
        }
        queue.push_back(tempCallback);
        queriesList.push_back(query);
        return &queue.back();
    }
    emit sendQuery(query,*conn);
    queue.push_back(tempCallback);
    return &queue.back();
}

void QtDatabase::receiveReply(const QSqlQuery &queryReturn)
{
    if(sqlQuery!=NULL)
    {
        delete sqlQuery;
        sqlQuery=NULL;
    }
    sqlQuery=new QSqlQuery(queryReturn);
    if(!queue.empty())
    {
        CallBack callback=queue.front();
        if(callback.method!=NULL)
            callback.method(callback.object);
        queue.erase(queue.begin());
    }
    if(sqlQuery!=NULL)
    {
        delete sqlQuery;
        sqlQuery=NULL;
    }
    clear();
    if(!queriesList.empty())
    {
        std::string query=queriesList.front();
        queriesList.erase(queriesList.begin());
        emit sendQuery(query,*conn);
    }
}

bool QtDatabase::asyncWrite(const std::string &query)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    QSqlQuery writeQuery(*conn);
    if(!writeQuery.exec(query.c_str()))
    {
        lastErrorMessage=(conn->lastError().driverText()+": "+conn->lastError().databaseText()).toUtf8().data();
        qDebug() << writeQuery.lastQuery()+": "+writeQuery.lastError().text();
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

const std::string QtDatabase::errorMessage() const
{
    if(conn==NULL)
        return lastErrorMessage;
    else
        return (conn->lastError().driverText()+": "+conn->lastError().databaseText()).toStdString();
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
        return false;
    }
    return true;
}

const std::string QtDatabase::value(const int &value) const
{
    if(sqlQuery==NULL)
        return emptyString;
    const std::string &string(sqlQuery->value(value).toString().toStdString());
    return string;
}

DatabaseBase::DatabaseType QtDatabase::databaseType() const
{
    return databaseConnected;
}

QtDatabaseThread::QtDatabaseThread()
{
    moveToThread(this);
    setObjectName("Db thread");
    start();
}

void QtDatabaseThread::receiveQuery(const std::string &query,const QSqlDatabase &db)
{
    QSqlQuery queryReturn(db);
    if(!queryReturn.exec(query.c_str()))
    {
        //lastErrorMessage=(db.lastError().driverText()+std::string(": ")+db.lastError().databaseText()).toUtf8().data();
        qDebug() << queryReturn.lastQuery()+": "+queryReturn.lastError().text();
    }
    emit sendReply(queryReturn);
}

bool QtDatabase::epollEvent(const uint32_t &events)
{
    Q_UNUSED(events);
    return false;
}
