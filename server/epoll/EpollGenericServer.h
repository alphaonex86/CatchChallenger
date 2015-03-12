#ifndef EPOLLGENERICSERVER_H
#define EPOLLGENERICSERVER_H

#include <sys/socket.h>

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
    int getSfd();
    Type getType() const;
private:
    int sfd;
};
}

#endif // EPOLLGENERICSERVER_H
