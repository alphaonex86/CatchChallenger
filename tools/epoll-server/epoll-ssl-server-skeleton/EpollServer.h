#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>

#include "BaseClassSwitch.h"

class EpollServer : public BaseClassSwitch
{
public:
    EpollServer();
    ~EpollServer();
    bool tryListen(char *port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType() const;
private:
    int sfd;
};

#endif // SERVER_H
