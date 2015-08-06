#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "../base/BaseServerLogin.h"
#include "EpollClientLoginSlave.h"

#include <QSettings>
#include <string>
#include <vector>
#include <curl/curl.h>

namespace CatchChallenger {
struct MemoryStruct {
  char *memory;
  size_t size;
};

class EpollServerLoginSlave : public EpollGenericServer
{
public:
    EpollServerLoginSlave();
    ~EpollServerLoginSlave();
    bool tryListen();
    void close();
    static QString httpMirrorFix(QString mirrors);
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
public:
    static EpollServerLoginSlave *epollServerLoginSlave;
    static CURL *curl;

    char * destination_server_ip;
    quint16 destination_server_port;
public:
    bool tcpNodelay,tcpCork;
    bool serverReady;
private:
    char * server_ip;
    char * server_port;
};
}

#endif

#endif // EPOLL_SERVER_H
