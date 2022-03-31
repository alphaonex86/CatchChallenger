#include "EpollUnixSocketServer.hpp"
#include "EpollSocket.hpp"
#include "Epoll.hpp"

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

bool EpollUnixSocketServer::tryListen(const char * const path)
{
    #ifdef CATCHCHALLENGER_CLASS_STATS
    std::cout << "EpollUnixSocketServer::tryListen(): " << __FILE__ << ":" << __LINE__ << std::endl;
    #endif
    if(sfd != -1)
    {
        std::cerr << "Can't create the unix socket, sfd != -1" << std::endl;
        return false;
    }

    if((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        sfd=-1;
        std::cerr << "Can't create the unix socket" << std::endl;
        return false;
    }

    struct sockaddr_un local;
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path,path);
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
    {
        const int s = EpollSocket::make_non_blocking(sfd);
        if(s == -1)
        {
            close();
            std::cerr << "Can't put in non blocking" << std::endl;
            return false;
        }
    }
    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN | EPOLLOUT | EPOLLET;
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD, sfd, &event) == -1)
    {
        close();
        std::cerr << "epoll_ctl error: " << errno << std::endl;
        return false;
    }
    #ifdef CATCHCHALLENGER_CLASS_STATS
    std::cout << "EpollUnixSocketServer::tryListen() ok " << sfd << ": " << __FILE__ << ":" << __LINE__ << std::endl;
    #endif
    return true;
}

void EpollUnixSocketServer::close()
{
    #ifdef CATCHCHALLENGER_CLASS_STATS
    std::cout << "EpollUnixSocketServer::close(): " << sfd << " " << __FILE__ << ":" << __LINE__ << std::endl;
    #endif
    if(sfd!=-1)
    {
        ::close(sfd);
        sfd=-1;
    }
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
