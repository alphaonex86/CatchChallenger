#include "QtDatabase.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <QSqlError>
#include <QDebug>

using namespace CatchChallenger;

char QtDatabase::emptyString[]={'\0'};

std::vector<QtDatabase::EstablishedConnexion> QtDatabase::establishedConnexionList;
unsigned int QtDatabase::establishedConnexionCount=0;

QtDatabase::QtDatabase() :
    conn(NULL),
    sqlQuery(NULL)
{
    connect(this,&QtDatabase::sendQuery,&dbThread,&QtDatabaseThread::receiveQuery,Qt::QueuedConnection);
    connect(&dbThread,&QtDatabaseThread::sendReply,this,&QtDatabase::receiveReply,Qt::QueuedConnection);
}

QtDatabase::~QtDatabase()
{
    if(sqlQuery!=NULL)
        delete sqlQuery;
    if(conn!=NULL)
    {
        findConnexionToClose(conn);
        conn=NULL;
    }
}

QSqlDatabase * QtDatabase::findConnexionToOpen(const std::string &host, const std::string &dbname,const DatabaseBase::DatabaseType &typeToSearch)
{
    unsigned int index=0;
    while(index<QtDatabase::establishedConnexionList.size())
    {
        QtDatabase::EstablishedConnexion &establishedConnexion=QtDatabase::establishedConnexionList.at(index);
        if(establishedConnexion.host==host && establishedConnexion.dbname==dbname && establishedConnexion.type==typeToSearch)
        {
            establishedConnexion.openCount++;
            return establishedConnexion.conn;
        }
        index++;
    }
    return NULL;
}

unsigned int QtDatabase::findConnexionToClose(const QSqlDatabase * const conn)
{
    unsigned int index=0;
    while(index<QtDatabase::establishedConnexionList.size())
    {
        QtDatabase::EstablishedConnexion &establishedConnexion=QtDatabase::establishedConnexionList.at(index);
        if(establishedConnexion.conn==conn)
        {
            if(establishedConnexion.openCount>0)
                establishedConnexion.openCount--;
            unsigned int count=establishedConnexion.openCount;
            if(count==0)
            {
                QtDatabase::establishedConnexionList.erase(QtDatabase::establishedConnexionList.begin()+index);
                establishedConnexion.conn->close();
                delete establishedConnexion.conn;
            }
            return count;
        }
        index++;
    }
    return 0;
}

bool QtDatabase::isConnected() const
{
    return conn!=NULL;
}

bool QtDatabase::syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password)
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

bool QtDatabase::syncConnectMysql(const std::string &host, const std::string &dbname, const std::string &user,const std::string &password)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifdef CATCHCHALLENGER_SOLO
    std::cerr << "Disconnected to incorrect database type for solo into QtDatabase class" << std::endl;
    abort();
    #endif
    #endif
    return syncConnect(host,dbname,user,password);
}

bool QtDatabase::syncConnectSqlite(const std::string &file)
{
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

bool QtDatabase::syncConnectPostgresql(const std::string &host,const std::string &dbname,const std::string &user,const std::string &password)
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

void QtDatabase::syncDisconnect()
{
    if(conn==NULL)
    {
        std::cerr << "db not connected" << std::endl;
        return;
    }
    findConnexionToClose(conn);
    conn=NULL;
    databaseTypeVar=DatabaseBase::DatabaseType::Unknown;
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
    if(queryReturn.lastError().type()!=QSqlError::NoError)
        std::cerr << "error: " << queryReturn.lastError().driverText().toStdString() << std::endl;
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
        qDebug() << db.lastError().driverText() << ": " << db.lastError().databaseText() << " -> " << queryReturn.lastQuery() << ": " << queryReturn.lastError().text();
        abort();
    }
    emit sendReply(queryReturn);
}

bool QtDatabase::epollEvent(const uint32_t &events)
{
    Q_UNUSED(events);
    return false;
}
