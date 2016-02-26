#ifndef EPOLL_SOCKET_H
#define EPOLL_SOCKET_H

namespace CatchChallenger {
class EpollSocket
{
public:
    static int make_non_blocking(int sfd);
    static int make_blocking(int sfd);
};
}

#endif // EPOLL_SOCKET_H
