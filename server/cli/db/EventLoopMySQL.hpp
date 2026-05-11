#ifdef CATCHCHALLENGER_DB_MYSQL
#ifndef CATCHCHALLENGER_EVENT_LOOP_MYSQL_H
#define CATCHCHALLENGER_EVENT_LOOP_MYSQL_H

#include <mysql/mysql.h>
#include <queue>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

#include "EventLoopDatabase.hpp"

#define CATCHCHALLENGER_MAXBDQUERIES 1024

class EventLoopMySQL : public EventLoopDatabase
{
public:
    EventLoopMySQL();
    ~EventLoopMySQL();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    bool syncConnectInternal(bool infinityTry=false);
    void syncDisconnect();
    void syncReconnect();
    CatchChallenger::DatabaseBaseCallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const std::string &query);
    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    CatchChallenger::DatabaseBaseCallBack * asyncPreparedRead(const std::string &query,char * const id,void * returnObject,CallBackDatabase method,const std::vector<std::string> &values);
    bool asyncPreparedWrite(const std::string &query,char * const id,const std::vector<std::string> &values);
    bool queryPrepare(const char *stmtName,const char *query,const int &nParams,const bool &store=true);
    #endif
    bool unixEvent(const uint32_t &events);
    void clear();
    bool sendNextQuery();
    const std::string errorMessage() const;
    bool next();
    const std::string value(const int &value) const;
    bool isConnected() const;
    BaseClassSwitch::EventLoopObjectType getType() const;
private:
    MYSQL *conn;
    MYSQL_ROW row;
    int tuleIndex;
    int ntuples;
    int nfields;
    MYSQL_RES *result;
    std::vector<CatchChallenger::DatabaseBaseCallBack> queue;
    std::vector<std::string> queriesList;
    bool started;
    static char emptyString[1];
    static CatchChallenger::DatabaseBaseCallBack emptyCallback;
    static CatchChallenger::DatabaseBaseCallBack tempCallback;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    char strCohost[255];
    char strCouser[255];
    char strCodatabase[255];
    char strCopass[255];
    unsigned int portCo;

    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    // Cache stmtName -> MYSQL_STMT* prepared on `conn`. Owned here;
    // freed in destructor with mysql_stmt_close().
    std::unordered_map<std::string,MYSQL_STMT *> preparedStatementMap;
    // Re-prepared on reconnect (mirrors the PG preparedStatementUnitList).
    struct PreparedStatementStore
    {
        std::string name;
        std::string query;
        int nParams;
    };
    std::vector<PreparedStatementStore> preparedStatementUnitList;
    #endif
};

#endif
#endif
