#include "EpollPostgresql.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "../Epoll.h"
#include "../../../general/base/GeneralVariable.h"

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
}

EpollPostgresql::~EpollPostgresql()
{
    if(result!=NULL)
        PQclear(result);
    if(conn!=NULL)
        PQfinish(conn);
}

BaseClassSwitch::Type EpollPostgresql::getType() const
{
    return BaseClassSwitch::Type::Database;
}

bool EpollPostgresql::isConnected() const
{
    return conn!=NULL && started;
}

EpollPostgresql::DatabaseBase::Type EpollPostgresql::databaseType() const
{
    return EpollPostgresql::DatabaseBase::Type::PostgreSQL;
}

bool EpollPostgresql::syncConnect(const char * const host, const char * const dbname, const char * const user, const char * const password)
{
    if(conn!=NULL)
    {
        std::cerr << "pg already connected" << std::endl;
        return false;
    }

    strcpy(strCoPG,"");
    if(strlen(dbname)>0)
    {
        strcat(strCoPG,"dbname=");
        strcat(strCoPG,dbname);
    }
    if(strlen(host)>0 && strcmp(host,"localhost")!=0)
    {
        strcat(strCoPG," host=");
        strcat(strCoPG,host);
    }
    if(strlen(user)>0)
    {
        strcat(strCoPG," user=");
        strcat(strCoPG,user);
    }
    if(strlen(password)>0)
    {
        strcat(strCoPG," password=");
        strcat(strCoPG,password);
    }
    std::cerr << "Connecting to postgresql: " << host << "..." << std::endl;
    return syncConnect(strCoPG);
}

bool EpollPostgresql::syncConnect(const char * const fullConenctString)
{
    conn=PQconnectdb(fullConenctString);
    ConnStatusType connStatusType=PQstatus(conn);
    if(connStatusType==CONNECTION_BAD)
    {
       std::cerr << "pg connexion not OK, retrying..." << std::endl;
       unsigned int index=0;
       while(index<considerDownAfterNumberOfTry && connStatusType==CONNECTION_BAD)
       {
           sleep(tryInterval);
           //std::cerr << "Connecting to postgresql ... (" << (index+1) << ")" << std::endl;
           conn=PQconnectdb(strCoPG);
           connStatusType=PQstatus(conn);
           index++;
       }
       if(connStatusType==CONNECTION_BAD)
        return false;
    }
    std::cerr << "Connected to postgresql" << std::endl;
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
        std::cerr << "epoll_ctl, adding socket" << std::endl;
        return false;
    }

    PQsetNoticeReceiver(conn,noticeReceiver,NULL);
    PQsetNoticeProcessor(conn,noticeProcessor,NULL);
    started=true;
    std::cerr << "Protocol version:" << PQprotocolVersion(conn) << std::endl;
    std::cerr << "Server version:" << PQserverVersion(conn) << std::endl;
    return true;
}

void EpollPostgresql::syncReconnect()
{
    if(conn!=NULL)
    {
        std::cerr << "pg already connected" << std::endl;
        return;
    }
    syncConnect(strCoPG);
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
}

CatchChallenger::DatabaseBase::CallBack * EpollPostgresql::asyncRead(const char * const query,void * returnObject, CallBackDatabase method)
{
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
        queue << tempCallback;
        queriesList << QString::fromUtf8(query);
        return &queue.last();
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queriesList << QString::fromUtf8(query);
    #endif
    const int &query_id=PQsendQuery(conn,query);
    if(query_id==0)
    {
        std::cerr << "query send failed: " << errorMessage() << std::endl;
        return NULL;
    }
    queue << tempCallback;
    return &queue.last();
}

bool EpollPostgresql::asyncWrite(const char * const query)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    if(queue.size()>0 || result!=NULL)
    {
        queue << emptyCallback;
        queriesList << QString::fromUtf8(query);
        return true;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queriesList << QString::fromUtf8(query);
    #endif
    const int &query_id=PQsendQuery(conn,query);
    if(query_id==0)
    {
        std::cerr << "query send failed" << std::endl;
        return false;
    }
    queue << emptyCallback;
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
                    if(queriesList.isEmpty())
                        std::cerr << "Query to database failed: " << errorMessage() << std::endl;
                    else
                        std::cerr << "Query to database failed: " << errorMessage() << qPrintable(queriesList.first()) << std::endl;
                    #else
                    std::cerr << "Query to database failed: " << errorMessage() << std::endl;
                    #endif
                    tuleIndex=0;
                }
                else
                    ntuples=PQntuples(result);
                if(!queue.isEmpty())
                {
                    CallBack callback=queue.first();
                    if(callback.method!=NULL)
                        callback.method(callback.object);
                    queue.removeFirst();
                }
                if(result!=NULL)
                    clear();
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(!queriesList.isEmpty())
                    queriesList.removeFirst();
                #endif
                if(!queriesList.isEmpty())
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    const int &query_id=PQsendQuery(conn,queriesList.first().toUtf8());
                    #else
                    const int &query_id=PQsendQuery(conn,queriesList.takeFirst().toUtf8());
                    #endif
                    if(query_id==0)
                    {
                        std::cerr << "query async send failed: " << errorMessage() << std::endl;
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

const char * EpollPostgresql::errorMessage() const
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

const char * EpollPostgresql::value(const int &value) const
{
    if(result==NULL || tuleIndex<0)
        return emptyString;
    return PQgetvalue(result,tuleIndex,value);
}
