#ifndef EPOLL_UNIX_SOCKET_CLIENT_FINAL_H
#define EPOLL_UNIX_SOCKET_CLIENT_FINAL_H

#include "EpollUnixSocketClient.h"


namespace CatchChallenger {
class EpollUnixSocketClientFinal : public EpollUnixSocketClient
{
public:
    EpollUnixSocketClientFinal(const int &infd);
    ~EpollUnixSocketClientFinal();
    void parseIncommingData();
};
}

#endif // EPOLL_UNIX_SOCKET_CLIENT_FINAL_H
