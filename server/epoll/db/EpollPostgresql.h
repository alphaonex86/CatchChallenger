#ifndef CATCHCHALLENGER_EPOLLPOSTGRESQL_H
#define CATCHCHALLENGER_EPOLLPOSTGRESQL_H

#include <postgresql-9.3/libpq-fe.h>
#include <QList>
#include <QString>

#include "../../base/DatabaseBase.h"
#include "../BaseClassSwitch.h"

#define CATCHCHALLENGER_MAXBDQUERIES 1024

class EpollPostgresql : public BaseClassSwitch, public CatchChallenger::DatabaseBase
{
public:
    EpollPostgresql();
    ~EpollPostgresql();
    Type getType() const;
    bool syncConnect(const char * host, const char * dbname, const char * user, const char * password);
    bool syncConnect(const char * fullConenctString);
    void syncDisconnect();
    void syncReconnect();
    CallBack * asyncRead(const char *query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const char *query);
    static void noticeReceiver(void *arg, const PGresult *res);
    static void noticeProcessor(void *arg, const char *message);
    bool epollEvent(const uint32_t &events);
    void clear();
    const char * errorMessage() const;
    bool next();
    const char * value(const int &value) const;
    bool isConnected() const;
private:
    PGconn *conn;
    int tuleIndex;
    int ntuples;
    PGresult *result;
    QList<CallBack> queue;
    QList<QString> queriesList;
    bool started;
    static char emptyString[1];
    static CallBack emptyCallback;
    static CallBack tempCallback;
    char strCoPG[255];
};

#endif
