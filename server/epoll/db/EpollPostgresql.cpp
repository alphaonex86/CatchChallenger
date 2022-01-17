#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#include "EpollPostgresql.hpp"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "../Epoll.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/cpp11addition.hpp"
#ifdef DEBUG_MESSAGE_CLIENT_SQL
#include "../../base/SqlFunction.hpp"
#endif
#include <chrono>
#include <ctime>
#include <thread>

std::string EpollPostgresql::emptyString;
CatchChallenger::DatabaseBaseCallBack EpollPostgresql::emptyCallback;
CatchChallenger::DatabaseBaseCallBack EpollPostgresql::tempCallback;
bool EpollPostgresql::informationDisplayed=false;

void EpollPostgresql::noticeReceiver(void *arg, const PGresult *res)
{
    (void)arg;
    (void)res;
}

void EpollPostgresql::noticeProcessor(void *arg, const char *message)
{
    (void)arg;
    (void)message;
}

EpollPostgresql::EpollPostgresql() :
    conn(NULL),
    tuleIndex(-1),
    ntuples(0),
    result(NULL),
    started(false)
{
    emptyCallback.object=NULL;
    emptyCallback.method=NULL;
    databaseTypeVar=DatabaseBase::DatabaseType::PostgreSQL;

    maxDbQueries=CATCHCHALLENGER_MAXBDQUERIES;

    /*queue.reserve(CATCHCHALLENGER_MAXBDQUERIES);
    queriesList.reserve(CATCHCHALLENGER_MAXBDQUERIES);*/

    if(EpollPostgresql::informationDisplayed==false)
    {
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        std::cout << "CATCHCHALLENGER_DB_PREPAREDSTATEMENT enabled" << std::endl;
        #else
        std::cout << "CATCHCHALLENGER_DB_PREPAREDSTATEMENT disabled" << std::endl;
        #endif
        EpollPostgresql::informationDisplayed=true;
    }
    startTime = time(NULL);
}

EpollPostgresql::~EpollPostgresql()
{
    if(result!=NULL)
    {
        PQclear(result);
        result=NULL;
    }
    if(conn!=NULL)
    {
        PQfinish(conn);
        conn=NULL;
    }
}

bool EpollPostgresql::setMaxDbQueries(const unsigned int &maxDbQueries)
{
    if(maxDbQueries<2)
        return false;
    this->maxDbQueries=maxDbQueries;
    return true;
}

BaseClassSwitch::EpollObjectType EpollPostgresql::getType() const
{
    return BaseClassSwitch::EpollObjectType::Database;
}

bool EpollPostgresql::isConnected() const
{
    return conn!=NULL && started;
}

bool EpollPostgresql::syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password)
{
    if(conn!=NULL)
    {
        std::cerr << "pg already connected" << std::endl;
        return false;
    }

    strcpy(strCoPG,"");
    if(dbname.size()>0)
    {
        strcat(strCoPG,"dbname=");
        strcat(strCoPG,dbname.c_str());
    }
    if(host.size()>0 && host!="localhost")
    {
        strcat(strCoPG," host=");
        strcat(strCoPG,host.c_str());
    }
    if(user.size()>0)
    {
        strcat(strCoPG," user=");
        strcat(strCoPG,user.c_str());
    }
    if(password.size()>0)
    {
        strcat(strCoPG," password=");
        strcat(strCoPG,password.c_str());
    }

    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    strcpy(simplifiedstrCoPG,"");
    if(dbname.size()>0)
    {
        strcat(simplifiedstrCoPG,"dbname=");
        strcat(simplifiedstrCoPG,dbname.c_str());
    }
    if(host.size()>0 && host!="localhost")
    {
        strcat(simplifiedstrCoPG," host=");
        strcat(simplifiedstrCoPG,host.c_str());
    }
    #endif

    std::cout << "Connecting to postgresql: " << host << " on " << dbname << " with user " << user << std::endl;
    return syncConnectInternal();
}

bool EpollPostgresql::syncConnectInternal(bool infinityTry)
{
    conn=PQconnectdb(strCoPG);
    ConnStatusType connStatusType=PQstatus(conn);
    if(connStatusType==CONNECTION_BAD)
    {
        std::string lastErrorMessage=errorMessage();
        if(!infinityTry)
        {
            if(lastErrorMessage.find("pg_hba.conf")!=std::string::npos)
                return false;
            if(lastErrorMessage.find("authentication failed")!=std::string::npos)
                return false;
            if(lastErrorMessage.find("role \"")!=std::string::npos)
            {
                if(lastErrorMessage.find("\" does not exist")!=std::string::npos)
                    return false;
                if(lastErrorMessage.find("\" is not permitted to log in")!=std::string::npos)
                    return false;
            }
        }
        std::cerr << "pg connexion not OK: " << lastErrorMessage << ", retrying..., infinityTry: " << std::to_string(infinityTry) << std::endl;

        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connStatusType==CONNECTION_BAD)
        {
            auto start = std::chrono::high_resolution_clock::now();
            PQfinish(conn);
            conn=PQconnectdb(strCoPG);
            connStatusType=PQstatus(conn);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            if(elapsed.count()<(uint32_t)tryInterval*1000 && connStatusType==CONNECTION_BAD)
            {
                std::string newErrorMessage=errorMessage();
                if(lastErrorMessage!=newErrorMessage)
                {
                    std::cerr << "pg connexion not OK: " << lastErrorMessage << ", retrying..." << std::endl;
                    lastErrorMessage=newErrorMessage;
                }
                const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            if(!infinityTry)
                index++;
            if(lastErrorMessage.find("the database system is starting up")!=std::string::npos || lastErrorMessage.find("the database system is shutting down")!=std::string::npos)
                index=0;
        }
        if(connStatusType==CONNECTION_BAD)
            return false;
    }
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    {
        unsigned int index=0;
        while(index<preparedStatementUnitList.size())
        {
            const PreparedStatementStore &preparedStatement=preparedStatementUnitList.at(index);
            queryPrepare(preparedStatement.name.c_str(),preparedStatement.query.c_str(),preparedStatement.nParams,false);
            index++;
        }
    }
    #endif
    if(!setBlocking(1))
    {
       std::cerr << "pg no blocking error" << std::endl;
       return false;
    }
    int sock = PQsocket(conn);
    if (sock < 0)
    {
       std::cerr << "pg no sock" << std::endl;
       return false;
    }
    /* Use no delay, will not be able to group tcp message because is ordened into a queue
     * Then the execution is on by one, and the RTT will slow down this */
    {
        int state = 1;
        if(setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
            std::cerr << "Unable to apply tcp no delay" << std::endl;
    }
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET;
    event.data.ptr = this;

    // add the socket to the epoll file descriptors
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD,sock,&event) != 0)
    {
        std::cerr << "epoll_ctl, adding socket error" << std::endl;
        return false;
    }

    PQsetNoticeReceiver(conn,noticeReceiver,NULL);
    PQsetNoticeProcessor(conn,noticeProcessor,NULL);
    started=true;
    std::cout << "Connected to postgresql, Protocol version: " << PQprotocolVersion(conn) << ", Server version:" << PQserverVersion(conn) << std::endl;
    startTime = time(NULL);
    return true;
}

void EpollPostgresql::syncReconnect()
{
    if(conn!=NULL)
    {
        std::cerr << "pg already connected" << std::endl;
        return;
    }
    syncConnectInternal(true);
    if(!queriesList.empty())
        sendNextQuery();
}

//return true if success
bool EpollPostgresql::setBlocking(const bool &val)
{
    return PQsetnonblocking(conn,val)==0;
}

void EpollPostgresql::syncDisconnect()
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return;
    }
    if(!setBlocking(0))
    {
       std::cerr << "pg blocking error: errno: " << errno << std::endl;
       //return; continue to try again
    }
    PQfinish(conn);
    conn=NULL;
}

#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
CatchChallenger::DatabaseBaseCallBack * EpollPostgresql::asyncPreparedRead(
        const std::string &query,char * const id,void * returnObject,CallBackDatabase method,const std::vector<std::string> &values)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return NULL;
    }
    size_t bufferSize=0;
    const char *paramValues[values.size()];
    {
        uint8_t index=0;
        while(index<values.size())
        {
            paramValues[index]=values.at(index).c_str();
            bufferSize++;
            bufferSize+=values.at(index).size();
            index++;
        }
    }
    tempCallback.object=returnObject;
    tempCallback.method=method;
    PreparedStatement tempQuery;
    tempQuery.id=id;
    tempQuery.paramValuesBuffer=NULL;
    tempQuery.paramValuesCount=static_cast<uint8_t>(values.size());
    tempQuery.query=query;
    if(queue.size()>0 || result!=NULL)
    {
        if(queue.size()>=maxDbQueries)
        {
            std::cerr << "pg queue full" << std::endl;
            return NULL;
        }
        tempQuery.paramValuesBuffer=new char[bufferSize+1];
        tempQuery.paramValues=(char **)malloc(sizeof(char *)*values.size());
        {
            size_t cursor=0;
            uint8_t index=0;
            while(index<values.size())
            {
                tempQuery.paramValues[index]=tempQuery.paramValuesBuffer+cursor;
                strcpy(tempQuery.paramValuesBuffer+cursor,values.at(index).c_str());
                cursor+=values.at(index).size()+1;
                index++;
            }
        }
        queue.push_back(tempCallback);
        queriesList.push_back(tempQuery);
        return &queue.back();
    }
    start = std::chrono::high_resolution_clock::now();
    queriesList.push_back(tempQuery);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    const unsigned int &stringlen=static_cast<unsigned int>(query.size());
    if(stringlen==0)
    {
        std::cerr << "[" << (time(NULL)-startTime) << "] query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << simplifiedstrCoPG << ", query " << CatchChallenger::SqlFunction::replaceSQLValues(query,values) << " at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
    #endif
    const int &query_id=PQsendQueryPrepared(conn,id,static_cast<int>(values.size()),paramValues, NULL, NULL, 0);
    if(query_id==0)
    {
        std::cerr << "[" << (time(NULL)-startTime) << "] query send failed: " << errorMessage() << std::endl;
        return NULL;
    }
    queue.push_back(tempCallback);
    return &queue.back();
}

bool EpollPostgresql::asyncPreparedWrite(const std::string &query,char * const id,const std::vector<std::string> &values)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    size_t bufferSize=0;
    const char *paramValues[values.size()];
    {
        uint8_t index=0;
        while(index<values.size())
        {
            paramValues[index]=values.at(index).c_str();
            bufferSize++;
            bufferSize+=values.at(index).size();
            index++;
        }
    }
    PreparedStatement tempQuery;
    tempQuery.id=id;
    tempQuery.paramValuesCount=static_cast<uint8_t>(values.size());
    tempQuery.paramValuesBuffer=NULL;
    tempQuery.query=query;
    if(queue.size()>0 || result!=NULL)
    {
        tempQuery.paramValuesBuffer=new char[bufferSize+1];
        tempQuery.paramValues=(char **)malloc(sizeof(char *)*values.size());
        {
            size_t cursor=0;
            uint8_t index=0;
            while(index<values.size())
            {
                tempQuery.paramValues[index]=tempQuery.paramValuesBuffer+cursor;
                strcpy(tempQuery.paramValuesBuffer+cursor,values.at(index).c_str());
                cursor+=values.at(index).size()+1;
                index++;
            }
        }
        queue.push_back(emptyCallback);
        queriesList.push_back(tempQuery);
        return true;
    }
    start = std::chrono::high_resolution_clock::now();
    queriesList.push_back(tempQuery);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    const int &stringlen=static_cast<int>(query.size());
    if(stringlen==0)
    {
        std::cerr << "[" << (time(NULL)-startTime) << "] query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << simplifiedstrCoPG << ", query " << CatchChallenger::SqlFunction::replaceSQLValues(query,values) << " at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
    #endif
    const int &query_id=PQsendQueryPrepared(conn,id,static_cast<int>(values.size()),paramValues, NULL, NULL, 0);
    if(query_id==0)
    {
        std::cerr << "[" << (time(NULL)-startTime) << "] query send failed" << std::endl;
        return false;
    }
    queue.push_back(emptyCallback);
    return true;
}

bool EpollPostgresql::queryPrepare(const char *stmtName,
                                     const char *query,const int &nParams,const bool &store)//return NULL if failed
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    //std::cout << "Prepare the name: " << stmtName << ", query: " << query << ", database: " << this << std::endl;
    PGresult *resprep = PQprepare(conn,stmtName,query,nParams,/*,paramTypes*/NULL);
    const auto &ret=PQresultStatus(resprep);
    if (ret != PGRES_COMMAND_OK)
    { //if failed quit
        #ifdef DEBUG_MESSAGE_CLIENT_SQL
        std::cerr << simplifiedstrCoPG << ", ";
        #endif
        std::cerr << "Problem to prepare the query: " << query << ", return code: " << ret << ", error message: " << PQerrorMessage(conn) << std::endl;
        abort();
        return false;
    }
    if(store)
    {
        PreparedStatementStore preparedStatementStore;
        preparedStatementStore.name=stmtName;
        preparedStatementStore.query=query;
        preparedStatementStore.nParams=nParams;
        preparedStatementUnitList.push_back(preparedStatementStore);
    }
    return true;
}
#endif

CatchChallenger::DatabaseBaseCallBack * EpollPostgresql::asyncRead(const std::string &query,void * returnObject,CallBackDatabase method)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return NULL;
    }
    tempCallback.object=returnObject;
    tempCallback.method=method;
    PreparedStatement tempQuery;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    tempQuery.id=NULL;
    tempQuery.paramValues=NULL;
    tempQuery.paramValuesBuffer=NULL;
    tempQuery.paramValuesCount=0;
    #endif
    tempQuery.query=query;
    if(queue.size()>0 || result!=NULL)
    {
        if(queue.size()>=maxDbQueries)
        {
            std::cerr << "pg queue full" << std::endl;
            return NULL;
        }
        queue.push_back(tempCallback);
        queriesList.push_back(tempQuery);
        return &queue.back();
    }
    start = std::chrono::high_resolution_clock::now();
    queriesList.push_back(tempQuery);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    const int &stringlen=static_cast<int>(query.size());
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << simplifiedstrCoPG << ", query " << query << " at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
    #endif
    const int &query_id=PQsendQuery(conn,query.c_str());
    if(query_id==0)
    {
        std::cerr << "query send failed: " << errorMessage() << std::endl;
        return NULL;
    }
    queue.push_back(tempCallback);
    return &queue.back();
}

bool EpollPostgresql::asyncWrite(const std::string &query)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    PreparedStatement tempQuery;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    tempQuery.id=NULL;
    tempQuery.paramValues=NULL;
    tempQuery.paramValuesBuffer=NULL;
    tempQuery.paramValuesCount=0;
    #endif
    tempQuery.query=query;
    if(queue.size()>0 || result!=NULL)
    {
        queue.push_back(emptyCallback);
        queriesList.push_back(tempQuery);
        return true;
    }
    start = std::chrono::high_resolution_clock::now();
    queriesList.push_back(tempQuery);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    const int &stringlen=static_cast<int>(query.size());
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << simplifiedstrCoPG << ", query " << query << " at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
    #endif
    const int &query_id=PQsendQuery(conn,query.c_str());
    if(query_id==0)
    {
        std::cerr << "query send failed" << std::endl;
        return false;
    }
    queue.push_back(emptyCallback);
    return true;
}

void EpollPostgresql::clear()
{
    while(result!=NULL)
    {
        PQclear(result);
        result=PQgetResult(conn);
    }
    ntuples=0;
    tuleIndex=-1;
}

bool EpollPostgresql::epollEvent(const uint32_t &events)
{
    if(conn==NULL)
    {
        std::cerr << "epollEvent() conn==NULL" << std::endl;
        return false;
    }

    const ConnStatusType &connStatusType=PQstatus(conn);
    if(connStatusType!=CONNECTION_OK)
    {
        if(connStatusType==CONNECTION_MADE)
        {
            started=true;
            std::cout << "Connexion CONNECTION_MADE" << std::endl;
        }
        else if(connStatusType==CONNECTION_STARTED)
        {
            started=true;
            std::cout << "Connexion CONNECTION_STARTED" << std::endl;
        }
        else if(connStatusType==CONNECTION_AWAITING_RESPONSE)
            std::cout << "Connexion CONNECTION_AWAITING_RESPONSE" << std::endl;
        else
        {
            if(connStatusType==CONNECTION_BAD)
            {
                started=false;
                std::cerr << "Connexion not ok: CONNECTION_BAD" << std::endl;
                //return false;
            }
            else if(connStatusType==CONNECTION_AUTH_OK)
                std::cerr << "Connexion not ok: CONNECTION_AUTH_OK" << std::endl;
            else if(connStatusType==CONNECTION_SETENV)
                std::cerr << "Connexion not ok: CONNECTION_SETENV" << std::endl;
            else if(connStatusType==CONNECTION_SSL_STARTUP)
                std::cerr << "Connexion not ok: CONNECTION_SSL_STARTUP" << std::endl;
            else if(connStatusType==CONNECTION_NEEDED)
                std::cerr << "Connexion not ok: CONNECTION_NEEDED" << std::endl;
            else
                std::cerr << "Connexion not ok: " << connStatusType << std::endl;
        }
    }
    if(connStatusType!=CONNECTION_BAD)
    {
        const PostgresPollingStatusType &postgresPollingStatusType=PQconnectPoll(conn);
        if(postgresPollingStatusType==PGRES_POLLING_FAILED)
        {
            std::cerr << "Connexion status: PGRES_POLLING_FAILED" << std::endl;
            return false;
        }
    }

    if(events & EPOLLIN)
    {
        const int PQconsumeInputVar=PQconsumeInput(conn);
        PGnotify *notify;
        while((notify = PQnotifies(conn)) != NULL)
        {
            std::cerr << "ASYNC NOTIFY of '" << notify->relname << "' received from backend PID " << notify->be_pid << std::endl;
            PQfreemem(notify);
        }
        if(/*PQisBusy(conn)==0, produce blocking, when server is unbusy this this never call*/true)
        {
            if(result!=NULL)
                clear();
            tuleIndex=-1;
            ntuples=0;
            result=PQgetResult(conn);
            if(result==NULL)
            {
                std::cerr << "[" << (time(NULL)-startTime) << "] ";
                if(!queriesList.empty())
                    std::cerr << queriesList.front().query << ", ";
                std::cerr << strCoPG << " query async send failed: " << errorMessage() << ", PQgetResult(conn) have returned NULL" << std::endl;
                time_t secs=time(0);
                tm *t=localtime(&secs);
                printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                    t->tm_year+1900,t->tm_mon+1,t->tm_mday,
                    t->tm_hour,t->tm_min,t->tm_sec);
            }
            else
            {
                auto end = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> elapsed = end-start;
                const uint32_t &ms=elapsed.count();
                if(ms>5000)
                {
                    if(queriesList.empty())
                        std::cerr << "[" << (time(NULL)-startTime) << "] " << "query too slow, take " << ms << "ms" << std::endl;
                    else
                        std::cerr << "[" << (time(NULL)-startTime) << "] " << queriesList.front().query << ": query too slow, take " << ms << "ms" << std::endl;
                }
                #ifdef DEBUG_MESSAGE_CLIENT_SQL
                else
                {
                    if(queriesList.empty())
                        std::cout << "[" << (time(NULL)-startTime) << "] " << "query take " << ms << "ms" << std::endl;
                    else
                        std::cout << "[" << (time(NULL)-startTime) << "] " << queriesList.front().query << ": query take " << ms << "ms" << std::endl;
                }
                #endif
                start = std::chrono::high_resolution_clock::now();
                while(result!=NULL)
                {
                    const ExecStatusType &execStatusType=PQresultStatus(result);
                    if(execStatusType!=PGRES_TUPLES_OK && execStatusType!=PGRES_COMMAND_OK)
                    {
                        #ifdef DEBUG_MESSAGE_CLIENT_SQL
                        std::cerr << simplifiedstrCoPG << ", ";
                        #endif
                        if(queriesList.empty())
                            std::cerr << "Query to database failed: " << errorMessage() << std::endl;
                        else
                            std::cerr << "Query to database failed: " << errorMessage() << queriesList.front().query << std::endl;
                        abort();//prevent continue running to prevent data corruption
                        tuleIndex=0;
                    }
                    else
                        ntuples=PQntuples(result);
                    if(!queue.empty())
                    {
                        CatchChallenger::DatabaseBaseCallBack callback=queue.front();
                        if(callback.method!=NULL)
                            callback.method(callback.object);
                        queue.erase(queue.cbegin());
                    }
                    if(result!=NULL)
                        clear();
                    if(!queriesList.empty())
                        queriesList.erase(queriesList.cbegin());
                    if(!queriesList.empty())
                        if(!sendNextQuery())
                            return false;
                    result=PQgetResult(conn);
                }
            }
        }
        else
            std::cout << "PostgreSQL events with EPOLLIN: PQisBusy: " << std::to_string(PQconsumeInputVar) << std::endl;
    }
    if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        std::cerr << strCoPG << "Database disconnected, events: " << events << std::endl;
        if(events | EPOLLRDHUP)
        {
            started=false;//if set this, try reconnect
            std::cerr << strCoPG << "Database disconnected, try reconnect: " << errorMessage() << std::endl;
            syncDisconnect();
            conn=NULL;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            syncReconnect();
        }
    }
    return true;
}

bool EpollPostgresql::sendNextQuery()
{
    const PreparedStatement &firstEntry=queriesList.front();
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    int query_id=0;
    if(firstEntry.id!=NULL)
        query_id=PQsendQueryPrepared(conn,firstEntry.id,firstEntry.paramValuesCount,firstEntry.paramValues, NULL, NULL, 0);
    else
        query_id=PQsendQuery(conn,firstEntry.query.c_str());
    #else
    const int &query_id=PQsendQuery(conn,firstEntry.query.c_str());
    #endif
    if(query_id==0)
    {
        std::string tempString;
        {
            unsigned int index=0;
            while(index<queriesList.size())
            {
                if(!tempString.empty())
                    tempString+=";";
                tempString+=queriesList.at(index).query;
                index++;
            }
        }
        #ifdef DEBUG_MESSAGE_CLIENT_SQL
        std::cerr << simplifiedstrCoPG << ", ";
        #endif
        std::cerr << "query async send failed: " << errorMessage() << ", where query list is not empty: " << tempString << std::endl;
        return false;
    }
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << simplifiedstrCoPG << ", query " << firstEntry.query << " from queue at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
    #endif
    return true;
}

const std::string EpollPostgresql::errorMessage() const
{
    return PQerrorMessage(conn);
}

bool EpollPostgresql::next()
{
    if(result==NULL)
        return false;
    if(tuleIndex+1<ntuples)
    {
        tuleIndex++;
        return true;
    }
    else
    {
        clear();
        return false;
    }
}

const std::string EpollPostgresql::value(const int &value) const
{
    if(result==NULL || tuleIndex<0)
        return emptyString;
    const auto &content=PQgetvalue(result,tuleIndex,value);
    if(content==NULL)
        return emptyString;
    return content;
}

bool EpollPostgresql::stringtobool(const std::string &string,bool *ok)
{
    #ifdef CATCHCHALLENGER_SERVER_TRUSTINTODATABASENUMBERRETURN
    if(ok!=NULL)
        *ok=true;
    if(string=="t")
        return true;
    else
        return false;
    #else
    if(string=="t")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    else if(string=="f")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return false;
    }
    #endif
}

std::vector<char> EpollPostgresql::hexatoBinary(const std::string &data,bool *ok)
{
    if(data.empty())
        return std::vector<char>();
    if(data.size()%2!=0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
           *ok=false;
        return std::vector<char>();
    }
    if(data.at(0)=='\\')
    {
        if(data.size()==2)
        {
            if(ok!=NULL)
               *ok=true;
            return std::vector<char>();
        }
        const std::string &hexaextract=data.substr(2,data.size()-2);
        #ifdef CATCHCHALLENGER_SERVER_TRUSTINTODATABASENUMBERRETURN
        if(hexaextract.size()%2!=0)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return std::vector<char>();
        }
        bool ok2;
        if(ok!=NULL)
            *ok=true;
        std::vector<char> out;
        out.reserve(hexaextract.length()/2);
        for(size_t i=0;i<hexaextract.length();i+=2)
        {
            const std::string &partpfchain=hexaextract.substr(i,2);
            const uint8_t &x=::hexToDecUnit(partpfchain,&ok2);
            if(!ok2)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
                #endif
                if(ok!=NULL)
                    *ok=false;
                return std::vector<char>();
            }
            out.push_back(x);
        }
        if(ok!=NULL)
            *ok=true;
        return out;
        #else
        return ::hexatoBinary(hexaextract,ok);
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_SERVER_TRUSTINTODATABASENUMBERRETURN
        if(data.size()%2!=0)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return std::vector<char>();
        }
        bool ok2;
        if(ok!=NULL)
            *ok=true;
        std::vector<char> out;
        out.reserve(data.length()/2);
        for(size_t i=0;i<data.length();i+=2)
        {
            const std::string &partpfchain=data.substr(i,2);
            const uint8_t &x=::hexToDecUnit(partpfchain,&ok2);
            if(!ok2)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
                #endif
                if(ok!=NULL)
                    *ok=false;
                return std::vector<char>();
            }
            out.push_back(x);
        }
        if(ok!=NULL)
            *ok=true;
        return out;
        #else
        return ::hexatoBinary(data,ok);
        #endif
    }
}

#endif
