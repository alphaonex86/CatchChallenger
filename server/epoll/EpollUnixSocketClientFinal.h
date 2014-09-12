#ifndef EPOLL_UNIX_SOCKET_CLIENT_FINAL_H
#define EPOLL_UNIX_SOCKET_CLIENT_FINAL_H

#include "EpollUnixSocketClient.h"

#ifdef SERVERBENCHMARK
#include <sys/time.h>
#endif

namespace CatchChallenger {
class EpollUnixSocketClientFinal : public EpollUnixSocketClient
{
public:
    EpollUnixSocketClientFinal(const int &infd);
    ~EpollUnixSocketClientFinal();
    void parseIncommingData();
    #ifdef SERVERBENCHMARK
    static timespec ts;
    static unsigned long long timeUsed;
    static unsigned long long timeUsedForUser;
    static unsigned long long timeUsedForTimer;
    static unsigned long long timeUsedForDatabase;
    #endif
};
}

#endif // EPOLL_UNIX_SOCKET_CLIENT_FINAL_H
