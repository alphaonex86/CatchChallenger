#ifndef EPOLL_CLIENT_H
#define EPOLL_CLIENT_H

#ifndef SERVERSSL

#include "BaseClassSwitch.hpp"
#include <sys/types.h>

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EpollClient : public BaseClassSwitch
{
public:
    EpollClient(const int &infd);
    ~EpollClient();
    void reopen(const int &infd);
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    EpollObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;

    void closeSocket();
    bool socketIsOpen();
    bool socketIsClosed();
protected:
    int infd;
};
}

#endif

#endif // EPOLL_CLIENT_H
