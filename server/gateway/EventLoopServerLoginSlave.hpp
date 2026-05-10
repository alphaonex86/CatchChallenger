#ifndef EVENTLOOP_SERVER_LOGIN_MASTER_H
#define EVENTLOOP_SERVER_LOGIN_MASTER_H

#include "../cli/EventLoopGenericServer.hpp"
#include "../base/BaseServer/BaseServerLogin.hpp"
#include "EventLoopClientLoginSlave.hpp"

#include <string>
#include <vector>

namespace CatchChallenger {
struct MemoryStruct {
  char *memory;
  size_t size;
  std::string fileName;
};

class EventLoopServerLoginSlave : public EventLoopGenericServer
{
public:
    EventLoopServerLoginSlave();
    ~EventLoopServerLoginSlave();
    bool tryListen();
    void close();
    static std::string httpMirrorFix(const std::string &mirrors);
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    unsigned int toUTF8WithHeader(const std::string &text,char * const data);
public:
    static EventLoopServerLoginSlave *unixServerLoginSlave;

    std::string destination_server_ip;
    uint16_t destination_server_port;
    char * destination_proxy_ip;
    uint16_t destination_proxy_port;
    uint8_t gatewayNumber;
public:
    bool tcpNodelay,tcpCork;
    bool serverReady;
private:
    char * server_ip;
    char * server_port;
};
}

#endif // EVENT_LOOP_SERVER_H
