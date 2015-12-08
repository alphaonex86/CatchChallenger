#ifndef EPOLL_SERVER_LOGIN_MASTER_H
#define EPOLL_SERVER_LOGIN_MASTER_H

#ifndef SERVERSSL

#include "../epoll/EpollGenericServer.h"
#include "../base/BaseServerLogin.h"
#include "EpollClientLoginSlave.h"

#include <string>
#include <vector>

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
    static std::string httpMirrorFix(const std::string &mirrors);
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
public:
    static EpollServerLoginSlave *epollServerLoginSlave;

    char * destination_server_ip;
    uint16_t destination_server_port;
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
