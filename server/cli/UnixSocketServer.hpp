#ifndef UNIX_SOCKET_SERVER_H
#define UNIX_SOCKET_SERVER_H

#include <sys/socket.h>

#include "BaseClassSwitch.hpp"

namespace CatchChallenger {
class UnixSocketServer : public BaseClassSwitch
{
public:
    UnixSocketServer();
    ~UnixSocketServer();
    bool tryListen(const char * const path);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    EventLoopObjectType getType() const;
private:
    int sfd;
    bool ready;
};
}

#endif // UNIX_SOCKET_SERVER_H
