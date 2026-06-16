#ifndef WIN32_COMPAT_HPP
#define WIN32_COMPAT_HPP

// win32_compat.hpp — the select()-platform compat shim. (Name is historical:
// it now covers every NON-epoll, select(2)-based target, not just win32.) It
// lets server/cli sources keep using the epoll(7) API names + plain
// ::close()/::read()/::write() for sockets regardless of platform.
//
// On Linux this is a thin shim: include the real <sys/epoll.h>, <unistd.h>,
// <sys/socket.h>, etc., and the helper inlines collapse to ::close()/errno.
//
// On the select()-based targets — Windows (_WIN32, winsock2), MS-DOS
// (__DJGPP__, Watt-32) and an RTOS+lwIP (CC_TARGET_ESP32) — there is no
// <sys/epoll.h>, so we pull in that platform's socket headers and SYNTHESISE
// the epoll_event struct + the EPOLL* mask bits the EventLoop façade uses.
// EventLoop.cpp's CATCHCHALLENGER_SELECT branch is the only consumer of these
// constants — select(2) is level-triggered, so EPOLLET is harmless (defined 0).
// Sharing one shim keeps the synthesised vocabulary defined once (a per-RTOS
// copy would only grow the code); a new select()-based RTOS adds one branch.
//
// Helpers (cc_*): one set per platform below.
//   cc_winsock_init() — once at program start (WSAStartup on Windows / Watt-32
//                       sock_init on DOS); no-op on Linux + ESP32. True on OK.
//   cc_close_socket(fd) — closesocket()/close_s()/::close() per platform.
//   cc_sock_errno()   — WSAGetLastError() on Windows, errno elsewhere.

// Windows, MS-DOS (DJGPP) and ESP32 (ESP-IDF/lwIP) are all select(2)-based and
// lack <sys/epoll.h>, so they SHARE the synthesised epoll_event + EPOLL* bits
// below; only the socket headers and the cc_* helpers differ. All of this is
// #ifdef-gated: the Linux epoll branch is byte-identical to before, so the
// native server binary does not grow.
#if defined(_WIN32) || defined(__DJGPP__) || defined(CC_TARGET_ESP32)

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

#elif defined(__DJGPP__) // MS-DOS: BSD sockets come from Watt-32, not the libc

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

#else // CC_TARGET_ESP32 — ESP-IDF lwIP BSD sockets over newlib; no <sys/epoll.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstdint>
#include <cerrno>
#include <sys/types.h>

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

#elif defined(__DJGPP__) // Watt-32 BSD socket helpers

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

#else // CC_TARGET_ESP32 — lwIP: WiFi/netif already up (app_main), sockets are
      // newlib-VFS fds, so ::close() works and errno is standard.

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

// ── ESP32 / lwIP netdb gaps ───────────────────────────────────────────────
// lwIP's <netdb.h> provides getnameinfo()/getaddrinfo() but is missing the
// NI_* flag macros and gai_strerror() that server/cli's EventLoopGenericServer
// and main-unix use for diagnostics. Pull in <netdb.h> here (so it is included
// before these definitions — win32_compat.hpp is included ahead of the bare
// <netdb.h> in both call sites), then fill the gaps. The values are the
// standard POSIX ones; getnameinfo on lwIP only honours NI_NUMERICHOST anyway.
// All gated on CC_TARGET_ESP32: no other target's translation is affected.
#if defined(CC_TARGET_ESP32)
#include <netdb.h>
#include <string.h>
#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST  0x01
#endif
#ifndef NI_NUMERICSERV
#define NI_NUMERICSERV  0x02
#endif
#ifndef NI_NOFQDN
#define NI_NOFQDN       0x04
#endif
#ifndef NI_NAMEREQD
#define NI_NAMEREQD     0x08
#endif
#ifndef NI_DGRAM
#define NI_DGRAM        0x10
#endif
#ifndef NI_MAXHOST
#define NI_MAXHOST      1025
#endif
#ifndef NI_MAXSERV
#define NI_MAXSERV      32
#endif
// gai_strerror() is not in lwIP. The call sites only log it; map to the plain
// errno string so a getaddrinfo failure still prints something useful.
inline const char *gai_strerror(int /*ecode*/)
{
    return ::strerror(errno);
}
#endif // CC_TARGET_ESP32

#endif // WIN32_COMPAT_HPP
