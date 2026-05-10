#include "EventLoopGenericServer.hpp"
#include "SocketUtil.hpp"
#include "EventLoop.hpp"
#include "win32_compat.hpp"

#include <iostream>
#ifndef _WIN32
#include <netdb.h>
#include <unistd.h>
#endif
#include <string.h>
#include <chrono>

using namespace CatchChallenger;

std::chrono::steady_clock::time_point EventLoopGenericServer::processStart=std::chrono::steady_clock::now();

EventLoopGenericServer::EventLoopGenericServer()
{
}

EventLoopGenericServer::~EventLoopGenericServer()
{
    close();
}

bool EventLoopGenericServer::tryListenInternal(const char* const ip,const char* const port)
{
    if(strlen(port)==0)
    {
        std::cout << "EventLoopGenericServer::tryListenInternal() port can't be empty (abort)" << std::endl;
        abort();
    }

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    if(ip==NULL || ip[0]=='\0')
    {
        s = getaddrinfo(NULL, port, &hints, &result);
        std::cerr << "getaddrinfo: NULL" << std::endl;
    }
    else
    {
        s = getaddrinfo(ip, port, &hints, &result);
        std::cerr << "getaddrinfo: ip: " << ip << std::endl;
    }
    if (s != 0)
    {
        std::cerr << "getaddrinfo:" << gai_strerror(s) << std::endl;
        return false;
    }

    close();
    rp = result;
    if(rp == NULL)
    {
        std::cerr << "rp == NULL, can't bind" << std::endl;
        return false;
    }
    unsigned int bindSuccess=0,bindFailed=0;
    while(rp != NULL)
    {
        const int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1)
        {
            std::cerr
                    << "unable to create the socket: familly: " << rp->ai_family
                    << ", rp->ai_socktype: " << rp->ai_socktype
                    << ", rp->ai_protocol: " << rp->ai_protocol
                    << ", rp->ai_addr: " << rp->ai_addr
                    << ", rp->ai_addrlen: " << rp->ai_addrlen
                    << std::endl;
            continue;
        }

        int one=1;
        if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,
                      reinterpret_cast<const char *>(&one), sizeof one)!=0)
            std::cerr << "Unable to apply SO_REUSEADDR" << std::endl;
#ifndef _WIN32
        one=1;
        if(setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof one)!=0)
            std::cerr << "Unable to apply SO_REUSEPORT" << std::endl;
#endif

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if(s!=0)
        {
            //unable to bind
            cc_close_socket(sfd);
            std::cerr
                    << "unable to bind: familly: " << rp->ai_family
                    << ", rp->ai_socktype: " << rp->ai_socktype
                    << ", rp->ai_protocol: " << rp->ai_protocol
                    << ", rp->ai_addr: " << rp->ai_addr
                    << ", rp->ai_addrlen: " << rp->ai_addrlen
                    << ", errno: " << std::to_string(errno)
                    << std::endl;
            bindFailed++;
        }
        else
        {
            if(sfd==-1)
            {
                std::cerr << "Leave without bind but s!=0" << std::endl;
                return false;
            }
            s = SocketUtil::make_non_blocking(sfd);
            if(s == -1)
            {
                close();
                std::cerr << "Can't put in non blocking" << std::endl;
                return false;
            }

            s = listen(sfd, SOMAXCONN);
            if(s == -1)
            {
                close();
                std::cerr << "Unable to listen" << std::endl;
                return false;
            }

            epoll_event event;
            memset(&event,0,sizeof(event));
            event.data.ptr = this;
            event.events = EPOLLIN | EPOLLOUT | EPOLLET;
            s = EventLoop::loop.ctl(EPOLL_CTL_ADD, sfd, &event);
            if(s == -1)
            {
                close();
                std::cerr << "epoll_ctl error" << std::endl;
                return false;
            }
            sfd_list.push_back(sfd);

            const std::chrono::system_clock::time_point p1 = std::chrono::system_clock::now();
            const long long bootMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now()-EventLoopGenericServer::processStart).count();
            std::cout << "[" << std::chrono::duration_cast<std::chrono::seconds>(
                   p1.time_since_epoch()).count() << "] "
                    << "correctly bind: familly: " << rp->ai_family
                    << ", rp->ai_socktype: " << rp->ai_socktype
                    << ", rp->ai_protocol: " << rp->ai_protocol
                    << ", rp->ai_addr: " << rp->ai_addr
                    << ", rp->ai_addrlen: " << rp->ai_addrlen
                    << ", port: " << port
                    << ", rp->ai_flags: " << rp->ai_flags
                    << ", boot: " << bootMs << "ms"
                    //<< ", rp->ai_canonname: " << rp->ai_canonname-> corrupt the output
                    << std::endl;
            char hbuf[NI_MAXHOST];
            if(!getnameinfo(rp->ai_addr, rp->ai_addrlen, hbuf, sizeof(hbuf),NULL, 0, NI_NAMEREQD))
                std::cout << "getnameinfo: " << hbuf << std::endl;
            bindSuccess++;
        }
        rp = rp->ai_next;
    }
    freeaddrinfo (result);
    return bindSuccess>0;
}

void EventLoopGenericServer::close()
{
    unsigned int index=0;
    while(index<sfd_list.size())
    {
        cc_close_socket(sfd_list.at(index));
        index++;
    }
    sfd_list.clear();
}

bool EventLoopGenericServer::isListening() const
{
    return !sfd_list.empty();
}

int EventLoopGenericServer::accept(sockaddr *in_addr,socklen_t *in_len)
{
    unsigned int index=0;
    while(index<sfd_list.size())
    {
        const int &sfd=sfd_list.at(index);
        const int &returnValue=::accept(sfd, in_addr, in_len);
        if(returnValue!=-1)
            return returnValue;
        index++;
    }
    return -1;
}

std::vector<int> EventLoopGenericServer::getSfd() const
{
    return sfd_list;
}

BaseClassSwitch::EventLoopObjectType EventLoopGenericServer::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Server;
}
