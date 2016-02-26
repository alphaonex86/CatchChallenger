#ifdef CATCHCHALLENGER_DB_POSTGRESQL
#ifndef CATCHCHALLENGER_EPOLLPOSTGRESQL_H
#define CATCHCHALLENGER_EPOLLPOSTGRESQL_H

#include <postgresql/libpq-fe.h>
#include <queue>
#include <vector>
#include <string>

#include "../../base/DatabaseBase.h"

#define CATCHCHALLENGER_MAXBDQUERIES 1024

class EpollPostgresql : public CatchChallenger::DatabaseBase
{
public:
    EpollPostgresql();
    ~EpollPostgresql();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    bool syncConnectInternal();
    void syncDisconnect();
    void syncReconnect();
    CallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const std::string &query);
    static void noticeReceiver(void *arg, const PGresult *res);
    static void noticeProcessor(void *arg, const char *message);
    bool epollEvent(const uint32_t &events);
    void clear();
    const std::string errorMessage() const;
    bool next();
    const std::string value(const int &value) const;
    bool isConnected() const;

    //over load of: CatchChallenger::DatabaseFunction
    bool stringtobool(const std::string &string,bool *ok=NULL);
private:
    PGconn *conn;
    int tuleIndex;
    int ntuples;
    PGresult *result;
    //vector more fast on small data with less than 1024<entry
    std::vector<CallBack> queue;
    std::vector<std::string> queriesList;
    bool started;
    static char emptyString[1];
    static CallBack emptyCallback;
    static CallBack tempCallback;
    char strCoPG[255];
};

#endif
#endif
