#ifdef CATCHCHALLENGER_DB_MYSQL
#include "EpollMySQL.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include "../Epoll.h"
#include "../EpollSocket.h"
#include "../../../general/base/GeneralVariable.h"
#include <chrono>
#include <ctime>
#include <thread>

char EpollMySQL::emptyString[]={'\0'};
CatchChallenger::DatabaseBase::CallBack EpollMySQL::emptyCallback;
CatchChallenger::DatabaseBase::CallBack EpollMySQL::tempCallback;

EpollMySQL::EpollMySQL() :
    conn(NULL),
    row(NULL),
    tuleIndex(-1),
    ntuples(0),
    nfields(0),
    result(NULL),
    started(false)
{
    emptyCallback.object=NULL;
    emptyCallback.method=NULL;
    databaseTypeVar=DatabaseBase::Type::Mysql;
}

EpollMySQL::~EpollMySQL()
{
    if(result!=NULL)
    {
        mysql_free_result(result);
        result=NULL;
    }
    row=NULL;
    if(conn!=NULL)
    {
        mysql_close(conn);
        conn=NULL;
    }
}

bool EpollMySQL::isConnected() const
{
    return conn!=NULL && started;
}

bool EpollMySQL::syncConnect(const char * const host, const char * const dbname, const char * const user, const char * const password)
{
    if(conn!=NULL)
    {
        std::cerr << "mysql already connected" << std::endl;
        return false;
    }

    strcpy(strCohost,host);
    strcpy(strCouser,user);
    strcpy(strCodatabase,dbname);
    strcpy(strCopass,password);
    std::cerr << "Connecting to mysql: " << host << "..." << std::endl;
    return syncConnectInternal();
}

bool EpollMySQL::syncConnectInternal()
{
    conn = mysql_init(NULL);
    if(conn == NULL)
    {
        std::cerr << "mysql unable to allocate the object" << std::endl;
        return false;
    }
    if(!mysql_real_connect(conn,strCohost,strCouser,strCopass,strCodatabase,3306,NULL,0))
    {
        std::cerr << "mysql connexion not OK, retrying..." << std::endl;

        bool connectionisbad=true;
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connectionisbad)
        {
            auto start = std::chrono::high_resolution_clock::now();
            mysql_close(conn);
            connectionisbad=mysql_real_connect(conn,strCohost,strCouser,strCopass,strCodatabase,3306,NULL,0);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            if(elapsed.count()<5*tryInterval && connectionisbad)
            {
                const unsigned int ms=5*tryInterval-elapsed.count();
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            index++;
        }
        if(connectionisbad)
        {
            std::cerr << "mysql unable to connect: " << mysql_error(conn) << ", errno: " << mysql_errno(conn) << std::endl;
            mysql_close(conn);
            conn=NULL;
            return false;
        }
    }
    std::cerr << "Connected to mysql" << std::endl;
    /*if(CatchChallenger::EpollSocket::make_non_blocking(conn->net.fd)!=0)
    {
       std::cerr << "mysql no blocking error" << std::endl;
       return false;
    }*/
    epoll_event event;
    event.events = EPOLLOUT | EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET;
    event.data.ptr = this;

    // add the socket to the epoll file descriptors
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD,conn->net.fd,&event) != 0)
    {
        std::cerr << "epoll_ctl, adding socket error" << std::endl;
        return false;
    }

    started=true;
    std::cerr << "Protocol version:" << mysql_get_proto_info(conn) << std::endl;
    std::cerr << "Server version:" << mysql_get_server_version(conn) << std::endl;
    return true;
}

void EpollMySQL::syncReconnect()
{
    if(conn!=NULL)
    {
        std::cerr << "mysql already connected" << std::endl;
        return;
    }
    syncConnectInternal();
}

void EpollMySQL::syncDisconnect()
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return;
    }
    if(CatchChallenger::EpollSocket::make_blocking(conn->net.fd)!=0)
    {
       std::cerr << "mysql blocking error" << std::endl;
       return;
    }
    mysql_close(conn);
    conn=NULL;
}

CatchChallenger::DatabaseBase::CallBack * EpollMySQL::asyncRead(const char * const query,void * returnObject, CallBackDatabase method)
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return NULL;
    }
    tempCallback.object=returnObject;
    tempCallback.method=method;
    if(queue.size()>0 || result!=NULL)
    {
        if(queue.size()>=CATCHCHALLENGER_MAXBDQUERIES)
        {
            std::cerr << "mysql queue full" << std::endl;
            return NULL;
        }
        queue << tempCallback;
        queriesList << QString::fromUtf8(query);
        return &queue.last();
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queriesList << QString::fromUtf8(query);
    #endif
    const int &stringlen=strlen(query);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    const int &query_id=mysql_send_query(conn,query,stringlen);
    if(query_id<0)
    {
        std::cerr << "query " << query << "send failed: " << errorMessage() << std::endl;
        return NULL;
    }
    queue << tempCallback;
    return &queue.last();
}

bool EpollMySQL::asyncWrite(const char * const query)
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
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
    const int &query_id=mysql_send_query(conn,query,strlen(query));
    if(query_id==0)
    {
        std::cerr << "query send failed" << std::endl;
        return false;
    }
    queue << emptyCallback;
    return true;
}

void EpollMySQL::clear()
{
    if(result!=NULL)
    {
        mysql_free_result(result);
        result=NULL;
    }
    row=NULL;
    ntuples=0;
    nfields=0;
    tuleIndex=-1;
}

bool EpollMySQL::epollEvent(const uint32_t &events)
{
    if(conn==NULL)
    {
        std::cerr << "epollEvent() conn==NULL" << std::endl;
        return false;
    }

    if(events & EPOLLIN)
    {
        if(0 == mysql_read_query_result(conn))
        {
            if(result!=NULL)
                clear();
            tuleIndex=-1;
            ntuples=0;
            nfields=0;
            result=mysql_store_result(conn);
            if(result!=NULL)
            {
                ntuples=mysql_num_rows(result);
                nfields=mysql_num_fields(result);
                if(!queue.isEmpty())
                {
                    CallBack callback=queue.first();
                    if(callback.method!=NULL)
                        callback.method(callback.object);
                    queue.removeFirst();
                }
                if(result!=NULL)
                    clear();
                if(!queriesList.isEmpty())
                    queriesList.removeFirst();
                if(!queriesList.isEmpty())
                {
                    const QByteArray &query=queriesList.first().toUtf8();
                    const int &query_id=mysql_send_query(conn,query.constData(),query.size());
                    if(query_id==0)
                    {
                        std::cerr << "query async send failed: " << errorMessage() << ", where query list is not empty: " << queriesList.join(";").toStdString() << std::endl;
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

const char * EpollMySQL::errorMessage() const
{
    return QString("%1, errno: %2").arg(mysql_error(conn)).arg(mysql_errno(conn)).toUtf8();
}

bool EpollMySQL::next()
{
    if(result==NULL)
        return false;
    if(tuleIndex+1<ntuples)
    {
        row=mysql_fetch_row(result);
        if(row!=NULL)
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
    else
    {
        clear();
        return false;
    }
}

const char * EpollMySQL::value(const int &value) const
{
    if(result==NULL || tuleIndex<0 || value>=nfields)
        return emptyString;
    return row[value];
}
#endif
