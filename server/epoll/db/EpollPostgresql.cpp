#include "EpollPostgresql.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include "../Epoll.h"

char EpollPostgresql::emptyString[]={'\0'};

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
    queueSize(0),
    tuleIndex(-1),
    result(NULL)
{
    unsigned int index=0;
    while(index<sizeof(queue))
    {
        queue[index].object=NULL;
        queue[index].method=NULL;
        index++;
    }
}

EpollPostgresql::~EpollPostgresql()
{
    if(result!=NULL)
        PQclear(result);
    if(conn!=NULL)
        PQfinish(conn);
}

bool EpollPostgresql::isConnected() const
{
    return conn!=NULL;
}

bool EpollPostgresql::syncConnect(const char * host, const char * dbname, const char * user, const char * password)
{
    if(conn!=NULL)
    {
        std::cerr << "pg already connected" << std::endl;
        return false;
    }

    char strCoPG[255];
    strcpy(strCoPG,"dbname=");
    strcat(strCoPG,dbname);
    if(strlen(host)>0)
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

    conn=PQconnectStart(strCoPG);
    const ConnStatusType &connStatusType=PQstatus(conn);
    if(connStatusType==CONNECTION_BAD)
    {
       std::cerr << "pg connexion not OK" << std::endl;
       return false;
    }
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
    event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET ;
    event.data.fd = sock;

    // add the socket to the epoll file descriptors
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD,sock,&event) != 0)
    {
        std::cerr << "epoll_ctl, adding socket" << std::endl;
        return false;
    }

    PQsetNoticeReceiver(conn,noticeReceiver,NULL);
    PQsetNoticeProcessor(conn,noticeProcessor,NULL);
    return true;
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

bool EpollPostgresql::asyncRead(const char *query,void * returnObject, CallBackDatabase method)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    if(queueSize>=256)
    {
        std::cerr << "pg queue full" << std::endl;
        return false;
    }
    int query_id=PQsendQuery(conn,query);
    if(query_id==0)
    {
        std::cerr << "query send failed" << std::endl;
        return false;
    }
    queue[queueSize].object=returnObject;
    queue[queueSize].method=method;
    queueSize++;
    return true;
}

bool EpollPostgresql::asyncWrite(const char *query)
{
    if(conn==NULL)
    {
        std::cerr << "pg not connected" << std::endl;
        return false;
    }
    if(queueSize>=256)
    {
        std::cerr << "pg queue full" << std::endl;
        return false;
    }
    int query_id=PQsendQuery(conn,query);
    if(query_id==0)
    {
        std::cerr << "query send failed" << std::endl;
        return false;
    }
    queue[queueSize].object=NULL;
    queue[queueSize].method=NULL;
    queueSize++;
    return true;
}

bool EpollPostgresql::readyToRead()
{
    const ConnStatusType &connStatusType=PQstatus(conn);
    if(connStatusType!=CONNECTION_OK)
    {
        if(connStatusType==CONNECTION_MADE)
            std::cout << "Connexion CONNECTION_MADE" << std::endl;
        else if(connStatusType==CONNECTION_STARTED)
            std::cout << "Connexion CONNECTION_STARTED" << std::endl;
        else
        {
            std::cerr << "Connexion not ok: " << connStatusType << std::endl;
            return false;
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
        {
            PQclear(result);
            result=NULL;
        }
        tuleIndex=-1;
        if(queue[0].method!=NULL)
            queue[0].method(queue[0].object);
        queueSize--;
        memmove(queue,queue+sizeof(CallBack *),queueSize*sizeof(CallBack *));
    }
    return true;
}

/*
for (i = 0; i < nFields; i++)
{
    int ptype = PQftype(result, i);
    std::cout << PQfname(result, i);
}
std::cout << "\n------------\n";
// next, print out the rows
for (i = 0; i < PQntuples(result); i++)
{
    for (int j = 0; j < nFields; j++)
        std::cout << PQgetvalue(result, i, j);
    std::cout << "\n";
}*/

char *EpollPostgresql::errorMessage()
{
    return PQerrorMessage(conn);
}

bool EpollPostgresql::next()
{
    if(result!=NULL)
        PQclear(result);
    if((result = PQgetResult(conn)) != NULL)
    {
        if(PQresultStatus(result) != PGRES_TUPLES_OK)
        {
            result=NULL;
            std::cout << "FETCH ALL failed: " << PQerrorMessage(conn) << std::endl;
            return false;
        }
        else
            tuleIndex++;
    }
    else
        return false;
    return true;
}

char * EpollPostgresql::value(const int &value)
{
    if(result==NULL || tuleIndex<0)
        return emptyString;
    return PQgetvalue(result,tuleIndex,value);
}
