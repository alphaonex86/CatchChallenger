#ifndef UNIX_SOCKET_CLIENT_FINAL_H
#define UNIX_SOCKET_CLIENT_FINAL_H

#include "UnixSocketClient.h"


namespace CatchChallenger {
class UnixSocketClientFinal : public UnixSocketClient
{
public:
    UnixSocketClientFinal(const int &infd);
    ~UnixSocketClientFinal();
    void parseIncommingData();
};
}

#endif // UNIX_SOCKET_CLIENT_FINAL_H
