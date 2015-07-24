#include "QtDatabase.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <QDebug>
#include <QSqlError>

using namespace CatchChallenger;

char QtDatabase::emptyString[]={'\0'};
QByteArray QtDatabase::valueReturnedData;

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
    databaseConnected=DatabaseBase::DatabaseType::Mysql;
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
        lastErrorMessage=(conn->lastError().driverText()+QString(": ")+conn->lastError().databaseText()).toUtf8().data();
        delete conn;
        conn=NULL;
        return false;
    }
    databaseConnected=DatabaseBase::DatabaseType::SQLite;
    return true;
}

bool QtDatabase::syncConnectPostgresql(const char * host, const char * dbname, const char * user, const char * password)
{
    if(conn!=NULL)
        syncDisconnect();
    conn = new QSqlDatabase();
    *conn = QSqlDatabase::addDatabase("QPSQL","server");
    QString tempString(host);
    if(tempString!=QStringLiteral("localhost"))
        conn->setHostName(host);
    conn->setDatabaseName(dbname);
    conn->setUserName(user);
    conn->setPassword(password);
    if(!conn->open())
    {
        lastErrorMessage=(conn->lastError().driverText()+QString(": ")+conn->lastError().databaseText()).toUtf8().data();
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

DatabaseBase::CallBack *QtDatabase::asyncRead(const char *query,void * returnObject, CallBackDatabase method)
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
        queue << tempCallback;
        queriesList << QString::fromUtf8(query);
        return &queue.last();
    }
    emit sendQuery(query,*conn);
    queue << tempCallback;
    return &queue.last();
}

void QtDatabase::receiveReply(const QSqlQuery &queryReturn)
{
    if(sqlQuery!=NULL)
    {
        delete sqlQuery;
        sqlQuery=NULL;
    }
    sqlQuery=new QSqlQuery(queryReturn);
    if(!queue.isEmpty())
    {
        CallBack callback=queue.first();
        if(callback.method!=NULL)
            callback.method(callback.object);
        queue.removeFirst();
    }
    if(sqlQuery!=NULL)
    {
        delete sqlQuery;
        sqlQuery=NULL;
    }
    clear();
    if(!queriesList.isEmpty())
        emit sendQuery(queriesList.takeFirst(),*conn);
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
        lastErrorMessage=(conn->lastError().driverText()+QString(": ")+conn->lastError().databaseText()).toUtf8().data();
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

const char *QtDatabase::errorMessage() const
{
    if(conn==NULL)
        return lastErrorMessage.toLatin1().constData();
    else
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
        return false;
    }
    return true;
}

const char * QtDatabase::value(const int &value) const
{
    if(sqlQuery==NULL)
        return emptyString;
    const QString &string(sqlQuery->value(value).toString());
    valueReturnedData=string.toUtf8();
    valueReturnedData[valueReturnedData.size()]='\0';
    return valueReturnedData.constData();
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

void QtDatabaseThread::receiveQuery(const QString &query,const QSqlDatabase &db)
{
    QSqlQuery queryReturn(db);
    if(!queryReturn.exec(query))
    {
        //lastErrorMessage=(db.lastError().driverText()+QString(": ")+db.lastError().databaseText()).toUtf8().data();
        qDebug() << QString(queryReturn.lastQuery()+": "+queryReturn.lastError().text());
    }
    emit sendReply(queryReturn);
}

bool QtDatabase::epollEvent(const uint32_t &events)
{
    Q_UNUSED(events);
    return false;
}
