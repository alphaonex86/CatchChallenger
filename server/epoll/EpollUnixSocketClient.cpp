#include "EpollUnixSocketClient.h"

#include <iostream>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <cstring>
#include <errno.h>
#include <string.h>
#include "../base/GlobalServerData.h"
#include "Epoll.h"
#include "EpollSocket.h"

using namespace CatchChallenger;

EpollUnixSocketClient::EpollUnixSocketClient(const int &infd) :
    infd(infd)
{
}

EpollUnixSocketClient::~EpollUnixSocketClient()
{
    close();
}

void EpollUnixSocketClient::close()
{
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        Epoll::epoll.ctl(EPOLL_CTL_DEL, infd, NULL);
        ::close(infd);
        //std::cout << "Closed connection on descriptor " << infd << std::endl;
        infd=-1;
    }
}

ssize_t EpollUnixSocketClient::read(char *buffer,const size_t &bufferSize)
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

ssize_t EpollUnixSocketClient::write(const char *buffer, const size_t &bufferSize)
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

BaseClassSwitch::Type EpollUnixSocketClient::getType() const
{
    return BaseClassSwitch::Type::UnixClient;
}

bool EpollUnixSocketClient::isValid() const
{
    return infd!=-1;
}

long int EpollUnixSocketClient::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    unsigned long int nbytes;
    // gives shorter than true amounts on Unix domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
        return nbytes;
    else
        return -1;
}
