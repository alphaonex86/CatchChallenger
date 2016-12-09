#ifdef CATCHCHALLENGER_DB_MYSQL
#include "EpollMySQL.h"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "../Epoll.h"
#include "../EpollSocket.h"
#include "../../../general/base/GeneralVariable.h"
#include "../../../general/base/cpp11addition.h"
#include "../../VariableServer.h"
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
    databaseTypeVar=DatabaseBase::DatabaseType::Mysql;

    queue.reserve(CATCHCHALLENGER_MAXBDQUERIES);
    queriesList.reserve(CATCHCHALLENGER_MAXBDQUERIES);
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

bool EpollMySQL::syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password)
{
    if(conn!=NULL)
    {
        std::cerr << "mysql already connected" << std::endl;
        return false;
    }

    strcpy(strCohost,host.c_str());
    strcpy(strCouser,user.c_str());
    strcpy(strCodatabase,dbname.c_str());
    strcpy(strCopass,password.c_str());
    std::cout << "Connecting to mysql: " << host << "..." << std::endl;
    return syncConnectInternal();
}

bool EpollMySQL::syncConnectInternal(bool infinityTry)
{
    conn = mysql_init(NULL);
    if(conn == NULL)
    {
        std::cerr << "mysql unable to allocate the object" << std::endl;
        return false;
    }
    if(!mysql_real_connect(conn,strCohost,strCouser,strCopass,strCodatabase,3306,NULL,0))
    {
        std::string lastErrorMessage=errorMessage();
        if(mysql_errno()==ER_ACCESS_DENIED_ERROR || mysql_sqlstate()==28000)
            return false;
        std::cerr << "mysql connexion not OK: " << lastErrorMessage << ", retrying..." << std::endl;

        bool connectionisbad=true;
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connectionisbad)
        {
            auto start = std::chrono::high_resolution_clock::now();
            mysql_close(conn);
            connectionisbad=mysql_real_connect(conn,strCohost,strCouser,strCopass,strCodatabase,3306,NULL,0);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            if(elapsed.count()<(uint32_t)tryInterval*1000 && connectionisbad)
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
        }
        if(connectionisbad)
        {
            std::cerr << "mysql unable to connect: " << mysql_error(conn) << ", errno: " << mysql_errno(conn) << std::endl;
            mysql_close(conn);
            conn=NULL;
            return false;
        }
    }
    std::cout << "Connected to mysql" << std::endl;
    /*if(CatchChallenger::EpollSocket::make_non_blocking(conn->net.fd)!=0)
    {
       std::cerr << "mysql no blocking error" << std::endl;
       return false;
    }*/
    /* Use no delay, will not be able to group tcp message because is ordened into a queue
     * Then the execution is on by one, and the RTT will slow down this */
    {
        int state = 1;
        if(setsockopt(conn->net.fd, IPPROTO_TCP, TCP_NODELAY, &state, sizeof(state))!=0)
            std::cerr << "Unable to apply tcp no delay" << std::endl;
    }
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
    std::cout << "Protocol version:" << mysql_get_proto_info(conn) << std::endl;
    std::cout << "Server version:" << mysql_get_server_version(conn) << std::endl;
    return true;
}

void EpollMySQL::syncReconnect()
{
    if(conn!=NULL)
    {
        std::cerr << "mysql already connected" << std::endl;
        return;
    }
    syncConnectInternal(true);
    if(!queriesList.empty())
        sendNextQuery();
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

CatchChallenger::DatabaseBase::CallBack * EpollMySQL::asyncRead(const std::string &query,void * returnObject, CallBackDatabase method)
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
        queue.push_back(tempCallback);
        queriesList.push_back(query);
        return &queue.back();
    }
    start = std::chrono::high_resolution_clock::now();
    queriesList.push_back(query);
    const int &stringlen=query.size();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << "host: " << strCohost << ", database: " << strCodatabase << ", query " << query << std::endl;
    #endif
    const int &query_id=mysql_send_query(conn,query.c_str(),stringlen);
    if(query_id<0)
    {
        std::cerr << "query " << query << "send failed: " << errorMessage() << std::endl;
        return NULL;
    }
    queue.push_back(tempCallback);
    return &queue.back();
}

bool EpollMySQL::asyncWrite(const std::string &query)
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return false;
    }
    if(queue.size()>0 || result!=NULL)
    {
        queue.push_back(emptyCallback);
        queriesList.push_back(query);
        return true;
    }
    start = std::chrono::high_resolution_clock::now();
    queriesList.push_back(query);
    const int &stringlen=query.size();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << "host: " << strCohost << ", database: " << strCodatabase << ", query " << query << std::endl;
    #endif
    const int &query_id=mysql_send_query(conn,query.c_str(),stringlen);
    if(query_id==0)
    {
        std::cerr << "query send failed" << std::endl;
        return false;
    }
    queue.push_back(emptyCallback);
    return true;
}

void EpollMySQL::clear()
{
    while(result!=NULL)
    {
        mysql_free_result(result);
        if(conn!=NULL)
            result=mysql_store_result(conn);
        else
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
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end-start;
            const uint32_t &ms=elapsed.count();
            if(ms>5000)
            {
                if(queriesList.empty())
                    std::cerr << "query too slow, take " << ms << "ms" << std::endl;
                else
                    std::cerr << queriesList.front() << ": query too slow, take " << ms << "ms" << std::endl;
            }
            start = std::chrono::high_resolution_clock::now();
            result=mysql_store_result(conn);
            if(result!=NULL)//SELECT
            {
                ntuples=mysql_num_rows(result);
                nfields=mysql_num_fields(result);
            }
            else//INSERT, UPDATE, DELETE
            {
                ntuples=0;
                nfields=0;
            }
            if(!queue.empty())
            {
                CallBack callback=queue.front();
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

bool EpollMySQL::sendNextQuery()
{
    const std::string &query=queriesList.front();
    const int &query_id=mysql_send_query(conn,query.c_str(),query.size());
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << strCoPG << ", query " << query << " from queue" << std::endl;
    #endif
    if(query_id!=0)
    {
        std::cerr << "query async send failed: " << errorMessage() << ", where query list is not empty: " << stringimplode(queriesList,';') << std::endl;
        return false;
    }
    return true;
}

const std::string EpollMySQL::errorMessage() const
{
    return mysql_error(conn)+std::string(", errno: ")+std::to_string(mysql_errno(conn));
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

const std::string EpollMySQL::value(const int &value) const
{
    if(result==NULL || tuleIndex<0 || value>=nfields)
        return emptyString;
    const auto &content=row[value];
    if(content==NULL)
        return emptyString;
    return content;
}
#endif
