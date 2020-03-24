#ifndef EPOLL_UNIX_SOCKET_CLIENT_H
#define EPOLL_UNIX_SOCKET_CLIENT_H

#include "BaseClassSwitch.h"
#include <sys/types.h>

namespace CatchChallenger {
class EpollUnixSocketClient : public BaseClassSwitch
{
public:
    EpollUnixSocketClient(const int &infd);
    ~EpollUnixSocketClient();
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    EpollObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
protected:
    int infd;
};
}

#endif // EPOLL_UNIX_SOCKET_CLIENT_H
