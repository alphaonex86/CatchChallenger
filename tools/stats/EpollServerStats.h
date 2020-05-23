#ifndef EPOLLSERVERSTATS_H
#define EPOLLSERVERSTATS_H

#include "../../server/epoll/EpollUnixSocketServer.hpp"

namespace CatchChallenger {
class EpollServerStats : public EpollUnixSocketServer
{
public:
    EpollServerStats();
    bool tryListen(const char * const path);
};
}

#endif // EPOLLSERVERSTATS_H
