
#include "EventLoopClient.hpp"
#include "win32_compat.hpp"

#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#endif
#include <cstring>
#include <errno.h>
#include <string.h>
#include "EventLoop.hpp"

using namespace CatchChallenger;

EventLoopClient::EventLoopClient(const int &infd) :
    infd(infd)
{
//    std::cerr << "EventLoopClient::EventLoopClient infd: " << infd << std::endl;
}

EventLoopClient::~EventLoopClient()
{
    close();
}

void EventLoopClient::reopen(const int &infd)
{
//    std::cerr << "EventLoopClient::reopen infd: " << infd << std::endl;
    close();
    this->infd=infd;
}

void EventLoopClient::close()
{
    //std::cerr << "EventLoopClient::close infd: " << infd << std::endl;
    if(infd!=-1)
    {
        char tempBuffer[4096];
        //purge the buffer to prevent multiple EPOLLIN due to level trigger (vs edge trigger)
        while(read(tempBuffer,sizeof(tempBuffer))>0)
        {}
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        EventLoop::loop.ctl(EPOLL_CTL_DEL, infd, NULL);
        cc_close_socket(infd);
        //std::cout << "Closed connection on descriptor " << infd << std::endl;
        infd=-1;
    }
}

void EventLoopClient::closeSocket()
{
    close();
}

bool EventLoopClient::socketIsOpen()
{
    return infd!=-1;
}

bool EventLoopClient::socketIsClosed()
{
    return !socketIsOpen();
}

ssize_t EventLoopClient::read(char *buffer,const size_t &bufferSize)
{
    //std::cerr << "EventLoopClient::read infd: " << infd << std::endl;
    if(infd==-1)
        return -1;
    const int64_t bytesAvailableVar=bytesAvailable();
    //need more performance? change the API for 0 copy API
    if(bytesAvailableVar<=0)//non blocking for read
    {
#ifdef _WIN32
        //Windows sockets are always set non-blocking via ioctlsocket()
        //in SocketUtil. Just call recv() and let WSAEWOULDBLOCK surface.
        const int r=::recv(static_cast<SOCKET>(infd),buffer,static_cast<int>(bufferSize),0);
        if(r==SOCKET_ERROR)
            return -1;
        return r;
#else
        //good alternative?: Not work
        /*if(errno == 11)
            return ::read(infd, buffer, bufferSize);*/

        //valid but can be slow:
        const int &flags = fcntl(infd, F_GETFL, 0);
        if(flags == -1)
        {
            std::cerr << "fcntl get flags error on " << infd << std::endl;
            return -1;
        }

        if(flags & O_NONBLOCK)
            return ::read(infd, buffer, bufferSize);
        return bytesAvailableVar;
#endif
    }
    ssize_t count;
#ifdef _WIN32
    const int want=(bytesAvailableVar>0 && bytesAvailableVar<(ssize_t)bufferSize)
                   ?static_cast<int>(bytesAvailableVar):static_cast<int>(bufferSize);
    const int r=::recv(static_cast<SOCKET>(infd),buffer,want,0);
    count=(r==SOCKET_ERROR)?-1:r;
#else
    if(bytesAvailableVar>0 && bytesAvailableVar<(ssize_t)bufferSize)
        count=::read(infd, buffer, bytesAvailableVar);
    else
        count=::read(infd, buffer, bufferSize);
#endif
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
    //std::cerr << "EventLoopClient::read infd: " << infd << ", count: " << count << std::endl;
    return count;
}

ssize_t EventLoopClient::write(const char *buffer, const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
#ifdef _WIN32
    const int sret=::send(static_cast<SOCKET>(infd),buffer,static_cast<int>(bufferSize),0);
    const ssize_t size=(sret==SOCKET_ERROR)?-1:sret;
#else
    const ssize_t size = ::write(infd, buffer, bufferSize);
#endif
    if(size != (ssize_t)bufferSize)
    {
        if(errno != EAGAIN)
        {
            if(errno!=104)
                std::cerr << "Write socket error, errno: " << errno << std::endl;
            else
                std::cerr << "Write socket error, Connection reset by peer (errno: 104)" << std::endl;
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

BaseClassSwitch::EventLoopObjectType EventLoopClient::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Client;
}

#ifdef CATCHCHALLENGER_IO_URING
int EventLoopClient::recvMultishotFd() const
{
    //Return -1 once the socket has been closed: re-arming against
    //a stale fd would either fail with -EBADF (best case) or arm
    //against a recycled fd belonging to a different client.
    return infd;
}
#endif

bool EventLoopClient::isValid() const
{
    return infd!=-1;
}

long int EventLoopClient::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    long int nbytes=0;
#ifdef _WIN32
    u_long avail=0;
    if(::ioctlsocket(static_cast<SOCKET>(infd),FIONREAD,&avail)==0)
    {
        nbytes=static_cast<long int>(avail);
#else
    // gives shorter than true amounts on EventLoop domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
    {
#endif
        if(nbytes<0 || nbytes>1024*1024*1024)
        {
            /*if(errno!=11)
                std::cerr << "ioctl(infd, FIONREAD, &nbytes) return incorrect value: " << nbytes << ", errno: " << errno << std::endl;*/
            return -1;
        }
        return nbytes;
    }
    else
        return -1;
}

