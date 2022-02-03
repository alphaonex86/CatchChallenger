#ifndef EPOLLSERVERSTATS_H
#define EPOLLSERVERSTATS_H

#include "../../server/epoll/BaseClassSwitch.h"

#include <netinet/in.h>
#include <vector>

namespace CatchChallenger {
class P2PServer : public BaseClassSwitch
{
public:
    P2PServer();
    bool tryListen(const char * const host, const uint16_t &port);
    EpollObjectType getType() const;
    bool tryListenInternal(const char* const ip,const char* const port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
private:
    std::vector<int> sfd_list;
};
}

#endif // EPOLLSERVERSTATS_H
