#ifdef SERVERNOSSL

#include "EpollClient.h"

#include <iostream>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <cstring>
#include "../base/GlobalServerData.h"
#include "Epoll.h"
#include "EpollSocket.h"

using namespace CatchChallenger;

EpollClient::EpollClient(const int &infd,const bool &tcpCork) :
    #ifndef SERVERNOBUFFER
    bufferSize(0),
    #endif
    infd(infd)
{
    #ifndef SERVERNOBUFFER
    memset(buffer,0,4096);
    #endif
    if(tcpCork)
    {
        //set cork for CatchChallener because don't have real time part
        int state = 1;
        if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
            std::cerr << "Unable to apply tcp cork" << std::endl;
    }
}

EpollClient::~EpollClient()
{
    close();
}

void EpollClient::close()
{
    #ifndef SERVERNOBUFFER
    bufferSize=0;
    #endif
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        Epoll::epoll.ctl(EPOLL_CTL_DEL, infd, NULL);
        ::close(infd);
        std::cout << "Closed connection on descriptor " << infd << std::endl;
        infd=-1;
    }
}

ssize_t EpollClient::read(char *buffer,const size_t &bufferSize)
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
    else if(count == 0)
    {
        /* End of file. The remote has closed the
        connection. */
        close();
        return -1;
    }
    return count;
}

ssize_t EpollClient::write(char *buffer,const size_t &bufferSize)
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
            #ifndef SERVERNOBUFFER
            if(this->bufferSize<BUFFER_MAX_SIZE)
            {
                if(size<0)
                {
                    if((this->bufferSize+bufferSize)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,bufferSize);
                        this->bufferSize+=bufferSize;
                        return bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                        return BUFFER_MAX_SIZE-this->bufferSize;
                    }
                }
                else
                {
                    const int &diff=bufferSize-size;
                    if((this->bufferSize+diff)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,diff);
                        this->bufferSize+=bufferSize;
                        return bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                        return BUFFER_MAX_SIZE-this->bufferSize;
                    }
                }
            }
            #else
            return size;
            #endif
        }
    }
    else
        return size;
}

void EpollClient::flush()
{
    #ifndef SERVERNOBUFFER
    if(bufferSize>0)
    {
        char buf[512];
        size_t count=512;
        if(bufferSize<count)
            count=bufferSize;
        memcpy(buf,buffer,count);
        ssize_t size = ::write(infd, buf, count);
        if(size<0)
        {
            if(errno != EAGAIN)
            {
                std::cerr << "Write socket buffer error" << std::endl;
                close();
            }
        }
        else
        {
            bufferSize-=size;
            memmove(buffer,buffer+size,bufferSize);
        }
    }
    #endif
}

BaseClassSwitch::Type EpollClient::getType() const
{
    return BaseClassSwitch::Type::Client;
}

bool EpollClient::isValid() const
{
    return infd!=-1;
}

long int EpollClient::bytesAvailable() const
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
#endif
