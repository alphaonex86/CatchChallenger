#ifdef CATCHCHALLENGER_DB_MYSQL
#include "EventLoopMySQL.hpp"

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "../EventLoop.hpp"
#include "../SocketUtil.hpp"
#include "../../../general/base/GeneralVariable.hpp"
#include "../../../general/base/cpp11addition.hpp"
#ifdef DEBUG_MESSAGE_CLIENT_SQL
#include "../../base/SqlFunction.hpp"
#endif
#include <chrono>
#include <ctime>
#include <thread>
#include <mysql/mysqld_error.h>

char EventLoopMySQL::emptyString[]={'\0'};
CatchChallenger::DatabaseBaseCallBack EventLoopMySQL::emptyCallback;
CatchChallenger::DatabaseBaseCallBack EventLoopMySQL::tempCallback;

EventLoopMySQL::EventLoopMySQL() :
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

EventLoopMySQL::~EventLoopMySQL()
{
    if(result!=NULL)
    {
        mysql_free_result(result);
        result=NULL;
    }
    row=NULL;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    {
        std::unordered_map<std::string,MYSQL_STMT *>::iterator it=preparedStatementMap.begin();
        while(it!=preparedStatementMap.end())
        {
            if(it->second!=NULL)
                mysql_stmt_close(it->second);
            ++it;
        }
        preparedStatementMap.clear();
    }
    #endif
    if(conn!=NULL)
    {
        mysql_close(conn);
        conn=NULL;
    }
}

bool EventLoopMySQL::isConnected() const
{
    return conn!=NULL && started;
}

bool EventLoopMySQL::syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password)
{
    if(conn!=NULL)
    {
        std::cerr << "mysql already connected" << std::endl;
        return false;
    }

    //"host" may carry a non-default port as "host:port". libpq has
    //its key=value conninfo syntax for this; libmysqlclient doesn't,
    //so we parse explicitly here. testingcluster.py runs ephemeral
    //mariadb on a non-default port and depends on this.
    std::string h=host;
    portCo=3306;
    {
        const size_t colon=h.find(':');
        if(colon!=std::string::npos)
        {
            const std::string p=h.substr(colon+1);
            h=h.substr(0,colon);
            const int parsed=atoi(p.c_str());
            if(parsed>0 && parsed<65536)
                portCo=static_cast<unsigned int>(parsed);
        }
    }
    strcpy(strCohost,h.c_str());
    strcpy(strCouser,user.c_str());
    strcpy(strCodatabase,dbname.c_str());
    strcpy(strCopass,password.c_str());
    std::cout << "Connecting to mysql: " << h << ":" << portCo << "..." << std::endl;
    return syncConnectInternal();
}

bool EventLoopMySQL::syncConnectInternal(bool infinityTry)
{
    conn = mysql_init(NULL);
    if(conn == NULL)
    {
        std::cerr << "mysql unable to allocate the object" << std::endl;
        return false;
    }
    if(!mysql_real_connect(conn,strCohost,strCouser,strCopass,strCodatabase,portCo,NULL,0))
    {
        std::string lastErrorMessage=errorMessage();
        if(mysql_errno(conn)==ER_ACCESS_DENIED_ERROR || strcmp(mysql_sqlstate(conn),"28000")==0)
            return false;
        std::cerr << "mysql connexion not OK: " << lastErrorMessage << ", retrying..." << std::endl;

        bool connectionisbad=true;
        unsigned int index=0;
        while(index<considerDownAfterNumberOfTry && connectionisbad)
        {
            std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
            mysql_close(conn);
            connectionisbad=mysql_real_connect(conn,strCohost,strCouser,strCopass,strCodatabase,3306,NULL,0);
            std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
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
    /*if(CatchChallenger::SocketUtil::make_non_blocking(conn->net.fd)!=0)
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
    if(EventLoop::loop.ctl(EPOLL_CTL_ADD,conn->net.fd,&event) != 0)
    {
        std::cerr << "epoll_ctl, adding socket error" << std::endl;
        return false;
    }

    started=true;
    std::cout << "Protocol version:" << mysql_get_proto_info(conn) << std::endl;
    std::cout << "Server version:" << mysql_get_server_version(conn) << std::endl;
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    // Replay prepared statements registered before the connection
    // bounce. queryPrepare with store=false avoids growing the unit
    // list on every reconnect.
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
    return true;
}

void EventLoopMySQL::syncReconnect()
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

BaseClassSwitch::EventLoopObjectType EventLoopMySQL::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Database;
}

void EventLoopMySQL::syncDisconnect()
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return;
    }
    if(CatchChallenger::SocketUtil::make_blocking(conn->net.fd)!=0)
    {
       std::cerr << "mysql blocking error" << std::endl;
       return;
    }
    mysql_close(conn);
    conn=NULL;
}

#if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
// Prepared statements on MySQL/MariaDB.
//
// libpq supports both the legacy text-protocol async path
// (PQsendQuery) AND PQsendQueryPrepared on the same socket; we drain
// both through epoll in unixEvent(). libmysqlclient (and libmariadb)
// expose two disjoint APIs: the text path (mysql_send_query +
// mysql_read_query_result, async-friendly) and the prepared path
// (mysql_stmt_execute, blocking only). Mixing them on one connection
// would force a synchronous round-trip every time we hit a prepared
// query and would break the epoll-driven queue model used by
// unixEvent().
//
// KISS: do NOT execute via mysql_stmt_*. We still call
// mysql_stmt_prepare() in queryPrepare() as a syntax/param-count
// validation step (and to surface the same errors PG would, early),
// then for asyncPreparedRead/Write we inline-substitute the $1..$N
// placeholders with mysql_real_escape_string-quoted values and ship
// the resulting plain query through asyncRead/asyncWrite. The
// statement-name -> MYSQL_STMT* map is held only so the validation
// survives across reconnect (it's re-prepared in syncConnectInternal,
// matching the PG behavior).

static bool eventLoopMysqlInlineSubstitute(MYSQL *conn,const std::string &query,const std::vector<std::string> &values,std::string &out)
{
    out=query;
    size_t cursor=0;
    size_t valueIndex=0;
    while(valueIndex<values.size())
    {
        const std::string placeholder=std::string("$")+std::to_string(valueIndex+1);
        const size_t pos=out.find(placeholder,cursor);
        if(pos==std::string::npos)
        {
            std::cerr << "mysql inline-substitution: placeholder " << placeholder << " not found in query: " << query << std::endl;
            return false;
        }
        const std::string &raw=values.at(valueIndex);
        char *escaped=(char *)malloc(raw.size()*2+1);
        if(escaped==NULL)
        {
            std::cerr << "malloc failed for escape buffer" << std::endl;
            return false;
        }
        const unsigned long escLen=mysql_real_escape_string(conn,escaped,raw.c_str(),raw.size());
        std::string replacement=std::string("'")+std::string(escaped,escLen)+std::string("'");
        free(escaped);
        out.replace(pos,placeholder.size(),replacement);
        cursor=pos+replacement.size();
        valueIndex++;
    }
    return true;
}

bool EventLoopMySQL::queryPrepare(const char *stmtName,const char *query,const int &nParams,const bool &store)
{
    (void)query;
    (void)nParams;
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return false;
    }
    //asyncPreparedRead/Write substitute $N placeholders inline and
    //send via the existing async send_query path — they NEVER call
    //mysql_stmt_execute. We skip mysql_stmt_prepare entirely because:
    //  1. mysql_stmt_prepare is a sync blocking call, fighting the
    //     "one async query per connection at a time" invariant the
    //     send_query path holds.
    //  2. libpq-style query strings often have $N placeholders inside
    //     quoted UNHEX('$1') and similar, which mysql_stmt_prepare
    //     refuses ("syntax error near '?'") — but inline substitution
    //     handles that fine (the substituted value goes inside the
    //     quotes).
    //The cache map still tracks names so re-prepare on reconnect is
    //a no-op; we just slot a NULL.
    std::string nameKey(stmtName);
    std::unordered_map<std::string,MYSQL_STMT *>::iterator existing=preparedStatementMap.find(nameKey);
    if(existing!=preparedStatementMap.end())
    {
        if(existing->second!=NULL)
            mysql_stmt_close(existing->second);
        preparedStatementMap.erase(existing);
    }
    preparedStatementMap[nameKey]=NULL;
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

CatchChallenger::DatabaseBaseCallBack * EventLoopMySQL::asyncPreparedRead(const std::string &query,char * const id,void * returnObject,CallBackDatabase method,const std::vector<std::string> &values)
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return NULL;
    }
    if(id==NULL)
    {
        std::cerr << "mysql asyncPreparedRead with NULL id, query: " << query << std::endl;
        return NULL;
    }
    // Optional sanity: the statement should have been prepared.
    const std::string nameKey(id);
    if(preparedStatementMap.find(nameKey)==preparedStatementMap.end())
    {
        std::cerr << "mysql prepared statement not registered: " << id << ", query: " << query << std::endl;
        return NULL;
    }
    std::string finalQuery;
    if(!eventLoopMysqlInlineSubstitute(conn,query,values,finalQuery))
        return NULL;
    return asyncRead(finalQuery,returnObject,method);
}

bool EventLoopMySQL::asyncPreparedWrite(const std::string &query,char * const id,const std::vector<std::string> &values)
{
    if(conn==NULL)
    {
        std::cerr << "mysql not connected" << std::endl;
        return false;
    }
    if(id==NULL)
    {
        std::cerr << "mysql asyncPreparedWrite with NULL id, query: " << query << std::endl;
        return false;
    }
    const std::string nameKey(id);
    if(preparedStatementMap.find(nameKey)==preparedStatementMap.end())
    {
        std::cerr << "mysql prepared statement not registered: " << id << ", query: " << query << std::endl;
        return false;
    }
    std::string finalQuery;
    if(!eventLoopMysqlInlineSubstitute(conn,query,values,finalQuery))
        return false;
    return asyncWrite(finalQuery);
}
#endif

CatchChallenger::DatabaseBaseCallBack * EventLoopMySQL::asyncRead(const std::string &query,void * returnObject, CallBackDatabase method)
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
    #ifdef CATCHCHALLENGER_HARDENED
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << "host: " << strCohost << ", database: " << strCodatabase << ", query " << query << " at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
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

bool EventLoopMySQL::asyncWrite(const std::string &query)
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
    #ifdef CATCHCHALLENGER_HARDENED
    if(stringlen==0)
    {
        std::cerr << "query " << query << ", stringlen==0" << std::endl;
        abort();
    }
    #endif
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << "host: " << strCohost << ", database: " << strCodatabase << ", query " << query << " at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
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

void EventLoopMySQL::clear()
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

bool EventLoopMySQL::unixEvent(const uint32_t &events)
{
    if(conn==NULL)
    {
        std::cerr << "unixEvent() conn==NULL" << std::endl;
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
            std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
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
        }
    }
    if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        if(EPOLLRDHUP)
        {
            started=false;
            std::cerr << "Database disconnected, try reconnect: " << errorMessage() << std::endl;
            syncDisconnect();
            conn=NULL;
            syncReconnect();
        }
    }
    return true;
}

bool EventLoopMySQL::sendNextQuery()
{
    const std::string &query=queriesList.front();
    const int &query_id=mysql_send_query(conn,query.c_str(),query.size());
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    std::cout << strCoPG << ", query " << query << " from queue at " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
    #endif
    if(query_id!=0)
    {
        std::cerr << "query async send failed: " << errorMessage() << ", where query list is not empty: " << stringimplode(queriesList,';') << std::endl;
        return false;
    }
    return true;
}

const std::string EventLoopMySQL::errorMessage() const
{
    return mysql_error(conn)+std::string(", errno: ")+std::to_string(mysql_errno(conn));
}

bool EventLoopMySQL::next()
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

const std::string EventLoopMySQL::value(const int &value) const
{
    if(result==NULL || tuleIndex<0 || value>=nfields)
        return emptyString;
    const char *content=row[value];
    if(content==NULL)
        return emptyString;
    return content;
}
#endif
