#ifndef EPOLL_UNIX_SOCKET_SERVER_H
#define EPOLL_UNIX_SOCKET_SERVER_H

#include <sys/socket.h>

#include "BaseClassSwitch.hpp"

namespace CatchChallenger {
class EpollUnixSocketServer : public BaseClassSwitch
{
public:
    EpollUnixSocketServer();
    ~EpollUnixSocketServer();
    bool tryListen(const char * const path);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    EpollObjectType getType() const;
private:
    int sfd;
    bool ready;
};
}

#endif // EPOLL_UNIX_SOCKET_SERVER_H
