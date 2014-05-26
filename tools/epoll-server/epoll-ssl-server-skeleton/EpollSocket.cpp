#include "EpollSocket.h"

#include <iostream>
#include <fcntl.h>

int EpollSocket::make_non_blocking(int sfd)
{
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1)
    {
        std::cerr << "fcntl get flags error" << std::endl;
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if(s == -1)
    {
        std::cerr << "fcntl set flags error" << std::endl;
        return -1;
    }

    return 0;
}
