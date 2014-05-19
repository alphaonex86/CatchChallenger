#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>

#include "BaseClassSwitch.h"
#include "../base/BaseServer.h"

class EpollServer : public BaseClassSwitch, public CatchChallenger::BaseServer
{
public:
    EpollServer();
    ~EpollServer();
    bool tryListen();
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType();
private:
    int sfd;
};

#endif // SERVER_H
