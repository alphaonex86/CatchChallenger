#ifdef CATCHCHALLENGER_DB_MYSQL
#ifndef CATCHCHALLENGER_EPOLLMYSQL_H
#define CATCHCHALLENGER_EPOLLMYSQL_H

#include <mysql/mysql.h>
#include <queue>
#include <vector>
#include <string>
#include <chrono>

#include "../../base/DatabaseBase.h"
#include "../BaseClassSwitch.h"

#define CATCHCHALLENGER_MAXBDQUERIES 1024

class EpollMySQL : public CatchChallenger::DatabaseBase
{
public:
    EpollMySQL();
    ~EpollMySQL();
    bool syncConnect(const std::string &host, const std::string &dbname, const std::string &user, const std::string &password);
    bool syncConnectInternal(bool infinityTry=false);
    void syncDisconnect();
    void syncReconnect();
    CallBack * asyncRead(const std::string &query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const std::string &query);
    bool epollEvent(const uint32_t &events);
    void clear();
    const std::string errorMessage() const;
    bool next();
    const std::string value(const int &value) const;
    bool isConnected() const;
private:
    MYSQL *conn;
    MYSQL_ROW row;
    int tuleIndex;
    int ntuples;
    int nfields;
    MYSQL_RES *result;
    std::vector<CallBack> queue;
    std::vector<std::string> queriesList;
    bool started;
    static char emptyString[1];
    static CallBack emptyCallback;
    static CallBack tempCallback;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;

    char strCohost[255];
    char strCouser[255];
    char strCodatabase[255];
    char strCopass[255];
};

#endif
#endif
