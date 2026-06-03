#ifndef WIN32_COMPAT_HPP
#define WIN32_COMPAT_HPP

// win32_compat.hpp — single header that lets server/cli sources keep using
// the epoll(7) API names + plain ::close()/::read()/::write() for sockets
// regardless of whether we are on Linux or Windows (MinGW).
//
// On Linux this is a thin shim: include the real <sys/epoll.h>, <unistd.h>,
// <sys/socket.h>, etc., and the helper inlines collapse to ::close()/errno.
//
// On Windows (_WIN32) we pull in <winsock2.h>/<ws2tcpip.h>/<windows.h>,
// then synthesise the epoll_event struct + the EPOLL* mask bits the
// EventLoop façade uses. EventLoop.cpp's CATCHCHALLENGER_SELECT branch is
// the only consumer of these constants — select(2) is level-triggered, so
// EPOLLET is harmless to OR-in (defined as 0).
//
// Helpers:
//   cc_winsock_init() — must be called once at program start on Windows
//                       (WSAStartup); no-op on Linux. Returns true on OK.
//   cc_close_socket(fd) — closesocket() on Windows, ::close() on Linux.
//   cc_sock_errno()   — WSAGetLastError() on Windows, errno on Linux.

// Windows and MS-DOS (DJGPP) are both select(2)-based and lack
// <sys/epoll.h>, so they SHARE the synthesised epoll_event + EPOLL* bits
// below; only the socket headers and the cc_* helpers differ. All of this is
// #ifdef-gated: the Linux epoll branch is byte-identical to before, so the
// native server binary does not grow.
#if defined(_WIN32) || defined(__DJGPP__)

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <cstdint>
#include <cerrno>

// MinGW's <sys/types.h> already provides ssize_t/off_t, but make sure.
#include <sys/types.h>

#else // __DJGPP__ — MS-DOS: BSD sockets come from Watt-32, not the libc

#include <tcp.h>          // Watt-32: sock_init() + BSD socket layer
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>    // Watt-32: ioctlsocket() + FIONBIO / FIONREAD
#include <unistd.h>
#include <cstdint>
#include <cerrno>
#include <sys/types.h>

// Watt-32's <sys/wtypes.h> defines legacy lowercase type macros
// (byte/word/dword); `byte` collides with C++17 std::byte and breaks the
// stdlib. We only needed the declarations above — drop the macros now so any
// STL header pulled in afterwards keeps std::byte intact.
#undef byte
#undef word
#undef dword

#endif

// Map Linux epoll event mask names onto distinct bit values. select(2) is
// level-triggered, so EPOLLET is defined as 0 (harmless when OR'd in).
#ifndef EPOLLIN
#define EPOLLIN     0x0001
#endif
#ifndef EPOLLPRI
#define EPOLLPRI    0x0002
#endif
#ifndef EPOLLOUT
#define EPOLLOUT    0x0004
#endif
#ifndef EPOLLERR
#define EPOLLERR    0x0008
#endif
#ifndef EPOLLHUP
#define EPOLLHUP    0x0010
#endif
#ifndef EPOLLRDHUP
#define EPOLLRDHUP  0x2000
#endif
#ifndef EPOLLET
#define EPOLLET     0
#endif

#ifndef EPOLL_CTL_ADD
#define EPOLL_CTL_ADD 1
#endif
#ifndef EPOLL_CTL_DEL
#define EPOLL_CTL_DEL 2
#endif
#ifndef EPOLL_CTL_MOD
#define EPOLL_CTL_MOD 3
#endif

typedef union epoll_data {
    void    *ptr;
    int      fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;

struct epoll_event {
    uint32_t     events;
    epoll_data_t data;
};

#if defined(_WIN32)

inline bool cc_winsock_init()
{
    WSADATA wsa;
    const int rc=WSAStartup(MAKEWORD(2,2),&wsa);
    return rc==0;
}

inline int cc_close_socket(int fd)
{
    return ::closesocket(static_cast<SOCKET>(fd));
}

inline int cc_sock_errno()
{
    return WSAGetLastError();
}

#else // __DJGPP__ — Watt-32 BSD socket helpers

// sock_init() brings up the Watt-32 stack (loads the packet driver, runs
// DHCP/BOOTP as configured); returns 0 on success.
inline bool cc_winsock_init()
{
    return sock_init()==0;
}

// Watt-32 closes a socket handle with close_s(); plain close() is for files.
inline int cc_close_socket(int fd)
{
    return close_s(fd);
}

inline int cc_sock_errno()
{
    return errno;
}

#endif

// Neither Windows (MinGW) nor Watt-32 has SO_REUSEPORT; SO_REUSEADDR is
// enough to share the listening port.
#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

#else // real epoll — Linux

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

inline bool cc_winsock_init()
{
    return true;
}

inline int cc_close_socket(int fd)
{
    return ::close(fd);
}

inline int cc_sock_errno()
{
    return errno;
}

#endif // select-target (Windows/DJGPP) vs Linux epoll

#endif // WIN32_COMPAT_HPP
