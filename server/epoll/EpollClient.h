#ifndef EPOLL_CLIENT_H
#define EPOLL_CLIENT_H

#ifndef SERVERSSL

#include "BaseClassSwitch.h"
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
    #ifndef SERVERNOBUFFER
    void flush();
    #endif
    Type getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
private:
    #ifndef SERVERNOBUFFER
    char buffer[BUFFER_MAX_SIZE];
    size_t bufferSize;
    #endif
    int infd;
};
}

#endif

#endif // EPOLL_CLIENT_H
