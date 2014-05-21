#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>

#include "BaseClassSwitch.h"

class Server : public BaseClassSwitch
{
public:
    Server();
    bool tryListen(char *port);
    void close();
    int accept(sockaddr *in_addr,socklen_t *in_len);
    int getSfd();
    Type getType();
private:
    int sfd;
};

#endif // SERVER_H
