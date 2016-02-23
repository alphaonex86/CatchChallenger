#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#include "EpollPostgresql.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "../Epoll.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/cpp11addition.h"
#include "../../VariableServer.h"
#include <chrono>
#include <ctime>
#include <thread>

char EpollPostgresql::emptyString[]={'\0'};
CatchChallenger::DatabaseBase::CallBack EpollPostgresql::emptyCallback;
CatchChallenger::DatabaseBase::CallBack EpollPostgresql::tempCallback;

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

    queue.reserve(CATCHCHALLENGER_MAXBDQUERIES);
    queriesList.reserve(CATCHCHALLENGER_MAXBDQUERIES);
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
    std::cout << "Connecting to postgresql: " << host << "..." << std::endl;
    return syncConnectInternal();
}

bool EpollPostgresql::syncConnectInternal()
{
    conn=PQconnectdb(strCoPG);
    ConnStatusType connStatusType=PQstatus(conn);
    if(connStatusType==CONNECTION_BAD)
    {
        std::cerr << "pg connexion not OK, retrying..." << std::endl;

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
                const unsigned int ms=(uint32_t)tryInterval*1000-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            index++;
        }
        if(connStatusType==CONNECTION_BAD)
            return false;
    }
    std::cout << "Connected to postgresql" << std::endl;
    if(PQsetnonblocking(conn,1)!=0)
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
    epoll_event event;
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
    std::cout << "Protocol version:" << PQprotocolVersion(conn) << std::endl;
    std::cout << "Server version:" << PQserverVersion(conn) << std::endl;
    return true;
}

void EpollPostgresql::syncReconnect()
{
    if(conn!=NULL)
    {
        std::cerr << "pg already connected" << std::endl;
        return;
    }
    syncConnectInternal();
}

void EpollPostgresql::syncDisconnect()
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return;
    }
    if(PQsetnonblocking(conn,0)!=0)
    {
       std::cerr << "pg blocking error" << std::endl;
       return;
    }
    PQfinish(conn);
    conn=NULL;
}

CatchChallenger::DatabaseBase::CallBack * EpollPostgresql::asyncRead(const std::string &query,void * returnObject,CallBackDatabase method)
{
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cerr << "query: " << query << std::endl;
    #endif
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return NULL;
    }
    tempCallback.object=returnObject;
    tempCallback.method=method;
    if(queue.size()>0 || result!=NULL)
    {
        if(queue.size()>=CATCHCHALLENGER_MAXBDQUERIES)
        {
            std::cerr << "pg queue full" << std::endl;
            return NULL;
        }
        queue.push_back(tempCallback);
        queriesList.push_back(query);
        return &queue.back();
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queriesList.push_back(query);
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
    if(queue.size()>0 || result!=NULL)
    {
        queue.push_back(emptyCallback);
        queriesList.push_back(query);
        return true;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queriesList.push_back(query);
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
        PQconsumeInput(conn);
        PGnotify *notify;
        while((notify = PQnotifies(conn)) != NULL)
        {
            std::cerr << "ASYNC NOTIFY of '" << notify->relname << "' received from backend PID " << notify->be_pid << std::endl;
            PQfreemem(notify);
        }
        if(PQisBusy(conn)==0)
        {
            if(result!=NULL)
                clear();
            tuleIndex=-1;
            ntuples=0;
            result=PQgetResult(conn);
            if(result!=NULL)
            {
                const ExecStatusType &execStatusType=PQresultStatus(result);
                if(execStatusType!=PGRES_TUPLES_OK && execStatusType!=PGRES_COMMAND_OK)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(queriesList.empty())
                        std::cerr << "Query to database failed: " << errorMessage() << std::endl;
                    else
                        std::cerr << "Query to database failed: " << errorMessage() << queriesList.front() << std::endl;
                    #else
                    std::cerr << "Query to database failed: " << errorMessage() << std::endl;
                    #endif
                    tuleIndex=0;
                }
                else
                    ntuples=PQntuples(result);
                if(!queue.empty())
                {
                    CallBack callback=queue.front();
                    if(callback.method!=NULL)
                        callback.method(callback.object);
                    queue.erase(queue.begin());
                }
                if(result!=NULL)
                    clear();
                if(!queriesList.empty())
                    queriesList.erase(queriesList.begin());
                if(!queriesList.empty())
                {
                    const int &query_id=PQsendQuery(conn,queriesList.front().c_str());
                    if(query_id==0)
                    {
                        std::cerr << "query async send failed: " << errorMessage() << ", where query list is not empty: " << stringimplode(queriesList,';') << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        started=false;
        if(EPOLLRDHUP)
        {
            std::cerr << "Database disconnected, try reconnect: " << errorMessage() << std::endl;
            syncDisconnect();
            conn=NULL;
            syncReconnect();
        }
    }
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
    return PQgetvalue(result,tuleIndex,value);
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
        if(ok!=NULL)
            *ok=false;
        return false;
    }
    #endif
}
#endif
