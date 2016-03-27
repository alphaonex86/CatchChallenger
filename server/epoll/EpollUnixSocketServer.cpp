#include "EpollUnixSocketServer.h"
#include "EpollSocket.h"
#include "Epoll.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace CatchChallenger;

EpollUnixSocketServer::EpollUnixSocketServer()
{
    ready=false;
    sfd=-1;
}

EpollUnixSocketServer::~EpollUnixSocketServer()
{
    close();
}

bool EpollUnixSocketServer::tryListen()
{
    abort();
    if(sfd != -1)
        return false;

    if((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        sfd=-1;
        std::cerr << "Can't create the unix socket" << std::endl;
        return false;
    }

    struct sockaddr_un local;
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, "/tmp/catchchallenger-server.sock");
    unlink(local.sun_path);
    int len = strlen(local.sun_path) + sizeof(local.sun_family);
    if(bind(sfd, (struct sockaddr *)&local, len)!=0)
    {
        close();
        std::cerr << "Can't bind the unix socket, error (errno): " << errno << ", error string: " << strerror(errno) << std::endl;
        return false;
    }

    if(listen(sfd, 5) == -1)
    {
        close();
        std::cerr << "Unable to listen, error (errno): " << errno << ", error string: " << strerror(errno) << std::endl;
        return false;
    }

    if(EpollSocket::make_non_blocking(sfd) == -1)
    {
        close();
        std::cerr << "Can't put in non blocking unix" << std::endl;
        return false;
    }

    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD, sfd, &event) == -1)
    {
        close();
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }
    return true;
}

void EpollUnixSocketServer::close()
{
    if(sfd!=-1)
        ::close(sfd);
}

int EpollUnixSocketServer::accept(sockaddr *in_addr,socklen_t *in_len)
{
    return ::accept(sfd, in_addr, in_len);
}

int EpollUnixSocketServer::getSfd()
{
    return sfd;
}

BaseClassSwitch::EpollObjectType EpollUnixSocketServer::getType() const
{
    return BaseClassSwitch::EpollObjectType::UnixServer;
}
