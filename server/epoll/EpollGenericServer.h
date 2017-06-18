#ifndef EPOLLGENERICSERVER_H
#define EPOLLGENERICSERVER_H

#include <sys/socket.h>
#include <vector>

#include "BaseClassSwitch.h"

namespace CatchChallenger {
class EpollGenericServer : public BaseClassSwitch
{
public:
    EpollGenericServer();
    ~EpollGenericServer();
    bool tryListenInternal(const char* const ip,const char* const port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    std::vector<int> getSfd() const;
    BaseClassSwitch::EpollObjectType getType() const;
    bool isListening() const;
private:
    std::vector<int> sfd_list;
};
}

#endif // EPOLLGENERICSERVER_H
