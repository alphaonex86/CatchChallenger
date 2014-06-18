#ifndef CATCHCHALLENGER_EPOLLPOSTGRESQL_H
#define CATCHCHALLENGER_EPOLLPOSTGRESQL_H

#include <postgresql-9.3/libpq-fe.h>
#include <QList>

#include "../../base/DatabaseBase.h"
#include "../BaseClassSwitch.h"

#define CATCHCHALLENGER_MAXBDQUERIES 256

class EpollPostgresql : public BaseClassSwitch
{
public:
    EpollPostgresql();
    ~EpollPostgresql();
    Type getType() const;
    bool syncConnect(const char * host, const char * dbname, const char * user, const char * password);
    void syncDisconnect();
    bool asyncRead(const char *query,void * returnObject,CallBackDatabase method);
    bool asyncWrite(const char *query);
    static void noticeReceiver(void *arg, const PGresult *res);
    static void noticeProcessor(void *arg, const char *message);
    bool readyToRead();
    char * errorMessage();
    bool next();
    char * value(const int &value);
    bool isConnected() const;

    struct CallBack
    {
        void *object;
        CallBackDatabase method;
    };
private:
    PGconn *conn;
    int tuleIndex;
    PGresult *result;
    QList<CallBack> queue;
    static char emptyString[1];
    bool started;
};

#endif
