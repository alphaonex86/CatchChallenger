#ifndef EPOLLSERVERSTATS_H
#define EPOLLSERVERSTATS_H

#include "../../server/epoll/EpollUnixSocketServer.hpp"
#include <string>

namespace CatchChallenger {
class EpollServerStats : public EpollUnixSocketServer
{
public:
    EpollServerStats();
    bool tryListen(const char * const path);
    bool reopen();
public:
    static EpollServerStats epollServerStats;
private:
    std::string path;
};
}

#endif // EPOLLSERVERSTATS_H
