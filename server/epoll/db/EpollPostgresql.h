#ifndef EPOLLPOSTGRESQL_H
#define EPOLLPOSTGRESQL_H

#include "../../base/AsyncDatabase.h"

class EpollPostgresql : public AsyncDatabase
{
public:
    void EpollPostgresql();
    bool syncConnect();
    void syncDisconnect();
    void asyncRead();
    void asyncWrite();
private:
    int efd;
};

#endif