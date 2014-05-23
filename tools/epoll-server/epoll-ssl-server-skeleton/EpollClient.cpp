#include "EpollClient.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

EpollClient::EpollClient(const int &infd) :
    #ifndef SERVERNOBUFFER
    bufferSize(0),
    #endif
    infd(infd)
{
    #ifndef SERVERNOBUFFER
    memset(buffer,0,4096);
    #endif
    //set cork for CatchChallener because don't have real time part
    int state = 1;
    if(setsockopt(infd, IPPROTO_TCP, TCP_CORK, &state, sizeof(state))!=0)
        perror("Unable to apply tcp cork");
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
        ::close(infd);
        printf("Closed connection on descriptor %d\n",infd);
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
            perror("read");
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
            perror("write");
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
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                    }
                }
                else
                {
                    const int &diff=bufferSize-size;
                    if((this->bufferSize+diff)<BUFFER_MAX_SIZE)
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,diff);
                        this->bufferSize+=bufferSize;
                    }
                    else
                    {
                        memcpy(this->buffer+this->bufferSize,buffer+size,BUFFER_MAX_SIZE-this->bufferSize);
                        this->bufferSize=BUFFER_MAX_SIZE;
                    }
                }
            }
            #endif
            return size;
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
        size_t count;
        char buf[512];
        count=512;
        if(bufferSize<count)
            count=bufferSize;
        memcpy(buf,buffer,count);
        ssize_t size = ::write(infd, buf, count);
        if(size<0)
        {
            if(errno != EAGAIN)
            {
                perror("write");
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
