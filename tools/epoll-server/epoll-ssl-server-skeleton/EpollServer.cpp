#include "EpollServer.h"
#include "EpollSocket.h"
#include "Epoll.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

EpollServer::EpollServer()
{
    sfd=-1;
}

EpollServer::~EpollServer()
{
    close();
}

bool EpollServer::tryListen(char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    memset(&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
    hints.ai_flags = AI_PASSIVE;     /* All interfaces */

    s = getaddrinfo (NULL, port, &hints, &result);
    if (s != 0)
    {
        fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
        return false;
    }

    for(rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if(sfd == -1)
        continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0)
        {
            /* We managed to bind successfully! */
            break;
        }

        ::close(sfd);
    }

    if(rp == NULL)
    {
        sfd=-1;
        fprintf(stderr, "Could not bind\n");
        return false;
    }

    freeaddrinfo (result);

    s = EpollSocket::make_non_blocking(sfd);
    if(s == -1)
    {
        sfd=-1;
        perror("can't put in non blocking");
        return false;
    }

    s = listen(sfd, SOMAXCONN);
    if(s == -1)
    {
        sfd=-1;
        perror("listen");
        return false;
    }

    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    s = Epoll::epoll.ctl(EPOLL_CTL_ADD, sfd, &event);
    if(s == -1)
    {
        sfd=-1;
        perror("epoll_ctl");
        return false;
    }
    return true;
}

void EpollServer::close()
{
    if(sfd!=-1)
        ::close(sfd);
}

int EpollServer::accept(sockaddr *in_addr,socklen_t *in_len)
{
    return ::accept(sfd, in_addr, in_len);
}

int EpollServer::getSfd()
{
    return sfd;
}

BaseClassSwitch::Type EpollServer::getType() const
{
    return BaseClassSwitch::Type::Server;
}
