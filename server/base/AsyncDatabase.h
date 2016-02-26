#ifndef ASYNCDATABASE_H
#define ASYNCDATABASE_H

class AsyncDatabase
{
public:
    bool syncConnect() = 0;
    void syncDisconnect() = 0;
    void asyncRead() = 0;
    void asyncWrite() = 0;
};

#endif