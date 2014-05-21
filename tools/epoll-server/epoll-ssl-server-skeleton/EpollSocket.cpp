#include "EpollSocket.h"

#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

int EpollSocket::make_non_blocking(int sfd)
{
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if(s == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}
