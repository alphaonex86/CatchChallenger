#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

namespace CatchChallenger {
class SocketUtil
{
public:
    static int make_non_blocking(int sfd);
    static int make_blocking(int sfd);
};
}

#endif // SOCKET_UTIL_H
