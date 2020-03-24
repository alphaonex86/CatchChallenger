#ifndef EPOLL_UNIX_SOCKET_CLIENT_FINAL_H
#define EPOLL_UNIX_SOCKET_CLIENT_FINAL_H

#include "EpollUnixSocketClient.h"

#ifdef SERVERBENCHMARK
#include <chrono>
#include <ctime>
#endif

namespace CatchChallenger {
class EpollUnixSocketClientFinal : public EpollUnixSocketClient
{
public:
    EpollUnixSocketClientFinal(const int &infd);
    ~EpollUnixSocketClientFinal();
    void parseIncommingData();
    #ifdef SERVERBENCHMARK
    static std::chrono::time_point<std::chrono::high_resolution_clock> start;
    static unsigned long long timeUsed;
    #ifdef SERVERBENCHMARKFULL
    static unsigned long long timeUsedForUser;
    static unsigned long long timeUsedForTimer;
    static unsigned long long timeUsedForDatabase;
    #endif
    #endif
};
}

#endif // EPOLL_UNIX_SOCKET_CLIENT_FINAL_H
