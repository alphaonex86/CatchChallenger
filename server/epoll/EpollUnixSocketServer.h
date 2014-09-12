#ifndef EPOLL_UNIX_SOCKET_SERVER_H
#define EPOLL_UNIX_SOCKET_SERVER_H

#include <sys/socket.h>

#include "BaseClassSwitch.h"

namespace CatchChallenger {
class EpollUnixSocketServer : public BaseClassSwitch
{
public:
    EpollUnixSocketServer();
    ~EpollUnixSocketServer();
    bool tryListen();
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType() const;
private:
    int sfd;
    bool ready;
};
}

#endif // EPOLL_UNIX_SOCKET_SERVER_H
