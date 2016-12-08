#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#ifndef CATCHCHALLENGER_EPOLLPOSTGRESQL_H
#define CATCHCHALLENGER_EPOLLPOSTGRESQL_H

#include <postgresql/libpq-fe.h>
#include <queue>
#include <vector>
#include <string>
#include <chrono>

#include "../../base/DatabaseBase.h"
#include "../../VariableServer.h"

#define CATCHCHALLENGER_MAXBDQUERIES 1024

class EpollPostgresql : public CatchChallenger::DatabaseBase
{
public:
    EpollPostgresql();
    ~EpollPostgresql();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    bool syncConnectInternal(bool infinityTry=false);
    void syncDisconnect();
    void syncReconnect();

    #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
    CallBack * asyncPreparedRead(const std::string &query,char * const id,void * returnObject,CallBackDatabase method,const std::vector<std::string> &values);
    bool asyncPreparedWrite(const std::string &query,char * const id,const std::vector<std::string> &values);
    bool queryPrepare(const char *stmtName,
                        const char *query, int nParams,
                        const Oid *paramTypes);//return NULL if failed
    #endif
    CallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const std::string &query);
    static void noticeReceiver(void *arg, const PGresult *res);
    static void noticeProcessor(void *arg, const char *message);
    bool epollEvent(const uint32_t &events);
    void clear();
    bool sendNextQuery();
    const std::string errorMessage() const;
    bool next();
    const std::string value(const int &value) const;
    bool isConnected() const;

    //over load of: CatchChallenger::DatabaseFunction
    bool stringtobool(const std::string &string,bool *ok=NULL);
    std::vector<char> hexatoBinary(const std::string &data,bool *ok=NULL);
private:
    PGconn *conn;
    int tuleIndex;
    int ntuples;
    PGresult *result;
    //vector more fast on small data with less than 1024<entry
    std::vector<CallBack> queue;
    struct PreparedStatement
    {
        std::string query;
        #if defined(CATCHCHALLENGER_DB_PREPAREDSTATEMENT)
        //null if not prepared
        const char * id;
        char ** paramValues;
        char * paramValuesBuffer;
        uint8_t paramValuesCount;
        #endif
    };
    std::vector<PreparedStatement> queriesList;
    bool started;
    static std::string emptyString;
    static CallBack emptyCallback;
    static CallBack tempCallback;
    char strCoPG[255];
    #ifdef DEBUG_MESSAGE_CLIENT_SQL
    char simplifiedstrCoPG[255];
    #endif
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    static bool informationDisplayed;
};

#endif
#endif
