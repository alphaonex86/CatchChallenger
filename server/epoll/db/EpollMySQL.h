#ifdef CATCHCHALLENGER_DB_MYSQL
#ifndef CATCHCHALLENGER_EPOLLMYSQL_H
#define CATCHCHALLENGER_EPOLLMYSQL_H

#include <mysql/mysql.h>
#include <queue>
#include <vector>
#include <string>

#include "../../base/DatabaseBase.h"
#include "../BaseClassSwitch.h"

#define CATCHCHALLENGER_MAXBDQUERIES 1024

class EpollMySQL : public BaseClassSwitch, public CatchChallenger::DatabaseBase
{
public:
    EpollMySQL();
    ~EpollMySQL();
    bool syncConnect(const char * const host, const char * const dbname, const char * const user, const char * const password);
    bool syncConnectInternal();
    void syncDisconnect();
    void syncReconnect();
    CallBack * asyncRead(const char * const query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const char * const query);
    bool epollEvent(const uint32_t &events);
    void clear();
    const char * errorMessage() const;
    bool next();
    const char * value(const int &value) const;
    bool isConnected() const;
private:
    MYSQL *conn;
    MYSQL_ROW row;
    int tuleIndex;
    int ntuples;
    int nfields;
    MYSQL_RES *result;
    std::vector<QueryInPogress> queue;
    bool started;
    static char emptyString[1];
    static CallBack emptyCallback;
    static CallBack tempCallback;

    char strCohost[255];
    char strCouser[255];
    char strCodatabase[255];
    char strCopass[255];
};

#endif
#endif
