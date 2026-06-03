#include "SocketUtil.hpp"
#include "win32_compat.hpp"

#include <iostream>
#ifndef _WIN32
#include <fcntl.h>
#endif

using namespace CatchChallenger;

int SocketUtil::make_non_blocking(int sfd)
{
#ifdef _WIN32
    u_long mode=1;
    if(::ioctlsocket(static_cast<SOCKET>(sfd),FIONBIO,&mode)!=0)
    {
        std::cerr << "ioctlsocket FIONBIO=1 error on " << sfd << std::endl;
        return -1;
    }
    return 0;
#elif defined(__DJGPP__)
    // Watt-32 sockets do not implement fcntl(F_GETFL/F_SETFL); use the BSD
    // ioctlsocket(FIONBIO) path, same as Windows.
    int mode=1;
    if(::ioctlsocket(sfd,FIONBIO,&mode)!=0)
    {
        std::cerr << "ioctlsocket FIONBIO=1 error on " << sfd << std::endl;
        return -1;
    }
    return 0;
#else
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1)
    {
        std::cerr << "fcntl get flags error on " << sfd << std::endl;
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if(s == -1)
    {
        std::cerr << "fcntl set flags error on " << sfd << std::endl;
        return -1;
    }

    return 0;
#endif
}

int SocketUtil::make_blocking(int sfd)
{
#ifdef _WIN32
    u_long mode=0;
    if(::ioctlsocket(static_cast<SOCKET>(sfd),FIONBIO,&mode)!=0)
    {
        std::cerr << "ioctlsocket FIONBIO=0 error" << std::endl;
        return -1;
    }
    return 0;
#elif defined(__DJGPP__)
    int mode=0;
    if(::ioctlsocket(sfd,FIONBIO,&mode)!=0)
    {
        std::cerr << "ioctlsocket FIONBIO=0 error" << std::endl;
        return -1;
    }
    return 0;
#else
    int flags, s;

    flags = fcntl(sfd, F_GETFL, 0);
    if(flags == -1)
    {
        std::cerr << "fcntl get flags error" << std::endl;
        return -1;
    }

    flags &= ~O_NONBLOCK;
    s = fcntl(sfd, F_SETFL, flags);
    if(s == -1)
    {
        std::cerr << "fcntl set flags error" << std::endl;
        return -1;
    }

    return 0;
#endif
}
