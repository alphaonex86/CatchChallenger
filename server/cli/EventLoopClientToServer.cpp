
#include "EventLoopClientToServer.hpp"

#include <iostream>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <cstring>
#include <errno.h>
#include <string.h>
#include "EventLoop.hpp"
#include "SocketUtil.hpp"

using namespace CatchChallenger;

EventLoopClientToServer::EventLoopClientToServer(const int &infd) :
    infd(infd)
{
}

EventLoopClientToServer::~EventLoopClientToServer()
{
    close();
}

void EventLoopClientToServer::close()
{
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        EventLoop::loop.ctl(EPOLL_CTL_DEL, infd, NULL);
        ::close(infd);
        //std::cout << "Closed connection on descriptor " << infd << std::endl;
        infd=-1;
    }
}

ssize_t EventLoopClientToServer::read(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const ssize_t &count=::read(infd, buffer, bufferSize);
    if(count == -1)
    {
        /* If errno == EAGAIN, that means we have read all
        data. So go back to the main loop. */
        if(errno != EAGAIN)
        {
            std::cerr << "Read socket error" << std::endl;
            close();
            return -1;
        }
    }
    return count;
}

ssize_t EventLoopClientToServer::write(const char *buffer, const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const ssize_t &size = ::write(infd, buffer, bufferSize);
    if(size != (ssize_t)bufferSize)
    {
        if(errno != EAGAIN)
        {
            std::cerr << "Write socket error" << std::endl;
            close();
            return -1;
        }
        else
        {
            std::cerr << "Write socket full: EAGAIN for size:" << bufferSize << std::endl;
            return size;
        }
    }
    else
        return size;
}

void EventLoopClientToServer::flush()
{
}

BaseClassSwitch::EventLoopObjectType EventLoopClientToServer::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Client;
}

bool EventLoopClientToServer::isValid() const
{
    return infd!=-1;
}

long int EventLoopClientToServer::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    unsigned long int nbytes;
    // gives shorter than true amounts on EventLoop domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
        return nbytes;
    else
        return -1;
}

