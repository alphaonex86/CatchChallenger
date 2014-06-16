#ifndef EPOLLPOSTGRESQL_H
#define EPOLLPOSTGRESQL_H

#include <postgresql-9.3/libpq-fe.h>

#include "../../base/DatabaseBase.h"

class EpollPostgresql
{
public:
    EpollPostgresql();
    ~EpollPostgresql();
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
    int queueSize;
    int tuleIndex;
    PGresult *result;
    CallBack queue[256];
    static char emptyString[1];
};

#endif
