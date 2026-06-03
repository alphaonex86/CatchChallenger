#include "TimerBroadcastAnnounce.hpp"
#include "../win32_compat.hpp"

#include <iostream>
#include <cstring>

// Socket headers. win32_compat.hpp already pulled in <winsock2.h> (Windows) or
// the Watt-32 BSD layer (DJGPP) or <sys/socket.h> (Linux). On Linux and ESP-IDF
// (lwIP) we additionally need sockaddr_in / htons / INADDR_BROADCAST.
#if !defined(_WIN32) && !defined(__DJGPP__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// UDP port the client LAN watcher (LanBroadcastWatcher) binds to.
static const uint16_t CC_LAN_BROADCAST_PORT=42490;

// Append a 16/32-bit value big-endian (QDataStream's default byte order).
static void cc_put_be16(std::vector<char> &v,uint16_t x)
{
    v.push_back(static_cast<char>((x>>8)&0xff));
    v.push_back(static_cast<char>(x&0xff));
}
static void cc_put_be32(std::vector<char> &v,uint32_t x)
{
    v.push_back(static_cast<char>((x>>24)&0xff));
    v.push_back(static_cast<char>((x>>16)&0xff));
    v.push_back(static_cast<char>((x>>8)&0xff));
    v.push_back(static_cast<char>(x&0xff));
}

TimerBroadcastAnnounce::TimerBroadcastAnnounce(const std::string &name,uint16_t serverPort) :
    fd(-1),
    destPort(CC_LAN_BROADCAST_PORT)
{
    // Build the Qt_4_4 datagram by hand (no Qt):
    //   QString name  -> quint32 byteCount(=2*len) + each char as UTF-16BE
    //   quint16 port  -> 2 bytes BE
    // ASCII/Latin-1 names only (one UTF-16 unit per byte); that matches the
    // server names used here and keeps this Qt-free.
    cc_put_be32(datagram,static_cast<uint32_t>(name.size())*2u);
    size_t i=0;
    while(i<name.size())
    {
        datagram.push_back(static_cast<char>(0x00));     // UTF-16BE high byte
        datagram.push_back(name[i]);                     // low byte
        i++;
    }
    cc_put_be16(datagram,serverPort);

#if defined(__DJGPP__)
    // MS-DOS: a single-NIC DOS box has no use for LAN auto-discovery and
    // Watt-32's broadcast story is fiddly; compile to an inert timer.
    (void)destPort;
#else
    const int s=static_cast<int>(::socket(AF_INET,SOCK_DGRAM,0));
    if(s<0)
    {
        std::cerr << "TimerBroadcastAnnounce: socket() failed errno=" << cc_sock_errno() << std::endl;
        return;
    }
    int yes=1;
    if(::setsockopt(s,SOL_SOCKET,SO_BROADCAST,
                    reinterpret_cast<const char *>(&yes),sizeof(yes))<0)
        std::cerr << "TimerBroadcastAnnounce: SO_BROADCAST failed errno=" << cc_sock_errno() << std::endl;
    fd=s;
    std::cout << "LAN announce enabled: \"" << name << "\" -> 255.255.255.255:"
              << destPort << " every tick" << std::endl;
#endif
}

TimerBroadcastAnnounce::~TimerBroadcastAnnounce()
{
    if(fd>=0)
    {
        cc_close_socket(fd);
        fd=-1;
    }
}

bool TimerBroadcastAnnounce::socketReady() const
{
    return fd>=0;
}

void TimerBroadcastAnnounce::exec()
{
#if !defined(__DJGPP__)
    if(fd<0)
        return;
    sockaddr_in dest;
    memset(&dest,0,sizeof(dest));
    dest.sin_family=AF_INET;
    dest.sin_port=htons(destPort);
    dest.sin_addr.s_addr=htonl(INADDR_BROADCAST);   // 255.255.255.255
    // Best-effort: a transient send failure is not fatal for an announce.
    ::sendto(fd,datagram.data(),static_cast<int>(datagram.size()),0,
             reinterpret_cast<const sockaddr *>(&dest),sizeof(dest));
#endif
}
