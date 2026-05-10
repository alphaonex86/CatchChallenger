#ifndef UNIX_SOCKET_CLIENT_H
#define UNIX_SOCKET_CLIENT_H

#include "BaseClassSwitch.h"
#include <sys/types.h>

namespace CatchChallenger {
class UnixSocketClient : public BaseClassSwitch
{
public:
    UnixSocketClient(const int &infd);
    ~UnixSocketClient();
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    EventLoopObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;
protected:
    int infd;
};
}

#endif // UNIX_SOCKET_CLIENT_H
