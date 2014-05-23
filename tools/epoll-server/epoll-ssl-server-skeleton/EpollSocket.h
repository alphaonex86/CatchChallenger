#ifndef EPOLL_SOCKET_H
#define EPOLL_SOCKET_H

class EpollSocket
{
public:
    static int make_non_blocking(int sfd);
};

#endif // EPOLL_SOCKET_H
