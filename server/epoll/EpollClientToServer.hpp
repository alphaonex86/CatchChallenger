#ifndef EPOLL_CLIENT_H
#define EPOLL_CLIENT_H

#ifndef SERVERSSL

#include "BaseClassSwitch.hpp"
#include <sys/types.h>

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EpollClientToServer : public BaseClassSwitch
{
public:
    EpollClientToServer(const int &infd);
    ~EpollClientToServer();
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    void flush();
    EpollObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
private:
    int infd;
};
}

#endif

#endif // EPOLL_CLIENT_H
