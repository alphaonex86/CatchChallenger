#include "QtDatabase.hpp"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <QSqlError>
#include <QDebug>
// Mirror this layer's qtDbStats_* atomics into the general/base/
// CCGuiStats surface so the admin GUI reads ONE set of counters
// regardless of which DB layer is active.  Header is a no-op when
// CATCHCHALLENGER_GUI_STATS is undefined, so non-GUI binaries don't
// pull anything in.
#include "../../../general/base/CCGuiStats.hpp"

using namespace CatchChallenger;

#ifdef CATCHCHALLENGER_CLASS_QT
// Definition of the GUI-only stats counters declared in QtDatabase.hpp.
// Zero-initialised; bumped on every async/sync query at the call site
// (and reset to zero per-tick by the GUI's polling code).
namespace CatchChallenger {
std::atomic<uint64_t> qtDbStats_queryTotalCount{0};
std::atomic<uint64_t> qtDbStats_replyTotalCount{0};
std::atomic<uint64_t> qtDbStats_lastQueryDurationNs{0};
}
#endif

char QtDatabase::emptyString[]={'\0'};

std::vector<QtDatabase::EstablishedConnexion> QtDatabase::establishedConnexionList;
unsigned int QtDatabase::establishedConnexionCount=0;

QtDatabase::QtDatabase() :
    conn(NULL),
    sqlQuery(NULL)
{
    setObjectName("QtDatabase");
    if(!connect(this,&QtDatabase::sendQuery,&dbThread,&QtDatabaseThread::receiveQuery,Qt::QueuedConnection))
        abort();
    if(!connect(&dbThread,&QtDatabaseThread::sendReply,this,&QtDatabase::receiveReply,Qt::QueuedConnection))
        abort();
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

DatabaseBaseCallBack *QtDatabase::asyncRead(const std::string &query,void * returnObject, CallBackDatabase method)
{
    std::cerr << "[SQL read] " << query << std::endl;
    #ifdef CATCHCHALLENGER_CLASS_QT
    qtDbStats_queryTotalCount.fetch_add(1, std::memory_order_relaxed);
    #endif
    #ifdef CATCHCHALLENGER_GUI_STATS
    // Same bump on the GUI-facing counter.
    CatchChallenger::gui_stats::db_query_total.fetch_add(1, std::memory_order_relaxed);
    #endif
    if(conn==NULL)
    {
        std::cerr << "db not connected" << std::endl;
        return NULL;
    }
    DatabaseBaseCallBack tempCallback;
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
    #ifdef CATCHCHALLENGER_CLASS_QT
    qtDbStats_replyTotalCount.fetch_add(1, std::memory_order_relaxed);
    #endif
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
        DatabaseBaseCallBack callback=queue.front();
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
    std::cerr << "[SQL exec-sync] " << query << std::endl;
    #ifdef CATCHCHALLENGER_CLASS_QT
    qtDbStats_queryTotalCount.fetch_add(1, std::memory_order_relaxed);
    const auto qStart = std::chrono::steady_clock::now();
    #endif
    if(conn==NULL)
    {
        std::cerr << "[SQL exec-sync] FAILED: db not connected (" << query << ")" << std::endl;
        return false;
    }
    QSqlQuery writeQuery(*conn);
    if(!writeQuery.exec(query.c_str()))
    {
        lastErrorMessage=(conn->lastError().driverText()+": "+conn->lastError().databaseText()).toUtf8().data();
        std::cerr << "[SQL exec-sync] FAILED: " << writeQuery.lastError().text().toStdString()
                  << " driver: " << conn->lastError().driverText().toStdString()
                  << " db: " << conn->lastError().databaseText().toStdString()
                  << " (" << query << ")" << std::endl;
        return false;
    }
    #ifdef CATCHCHALLENGER_CLASS_QT
    {
        // Capture sync-write duration for the GUI's "SQL latency" tile.
        // memory_order_relaxed: the GUI poll only reads the LAST value
        // it ever sees, no ordering against other writes is required.
        const auto qEnd = std::chrono::steady_clock::now();
        const auto durNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(qEnd - qStart).count();
        qtDbStats_lastQueryDurationNs.store(durNs, std::memory_order_relaxed);
        #ifdef CATCHCHALLENGER_GUI_STATS
        CatchChallenger::gui_stats::db_query_last_duration_ns.store(
            durNs, std::memory_order_relaxed);
        #endif
    }
    #endif
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
    //produce bug, if you need performance optimised, then user server epoll cli version
    //moveToThread(this);
    setObjectName("Db thread");
    //start();
}

QtDatabaseThread::~QtDatabaseThread()
{
    exit();
    quit();
    wait();
}

void QtDatabaseThread::receiveQuery(const std::string &query,const QSqlDatabase &db)
{
    QSqlQuery queryReturn(db);

    //to debug crash into queryReturn.exec(query.c_str())
    const char * const tempString=query.c_str();
    std::cerr << "[SQL exec] " << tempString << std::endl;

    if(!queryReturn.exec(tempString))
    {
        //lastErrorMessage=(db.lastError().driverText()+std::string(": ")+db.lastError().databaseText()).toUtf8().data();
        qDebug() << db.lastError().driverText() << ": " << db.lastError().databaseText() << " -> " << queryReturn.lastQuery() << ": " << queryReturn.lastError().text();
        abort();
    }
    emit sendReply(queryReturn);
}

bool QtDatabase::unixEvent(const uint32_t &events)
{
    Q_UNUSED(events);
    return false;
}
