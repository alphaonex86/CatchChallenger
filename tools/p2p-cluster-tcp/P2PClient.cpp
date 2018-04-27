#include "P2PClient.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <sys/epoll.h>
#include <strings.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "../../server/epoll/Epoll.h"
#include "../../general/base/GeneralVariable.h"

char P2PClient::readBuffer[];
std::vector<std::pair<std::string,uint16_t> > P2PClient::hostToConnect;

P2PClient::P2PClient(const int &infd) :
    infd(infd)
{
    memset(header,0,sizeof(header));

}

P2PClient::~P2PClient()
{
    close();
}

BaseClassSwitch::EpollObjectType P2PClient::getType() const
{
    return BaseClassSwitch::EpollObjectType::ClientP2P;
}

void P2PClient::parseIncommingData()
{
    if(header[0]==4)
        tryGetData();
    const ssize_t size=read(header+1,4-header[0]);
    if(!sendHello)
    {
        char toSend[]="0000Reply";
        const uint32_t dataSize=sizeof(toSend)-sizeof(uint32_t);
        memcpy(toSend,&dataSize,sizeof(dataSize));
        write(toSend,sizeof(toSend));
        sendHello=true;
    }
    if(size<=0)
        return;
    header[0]+=size;
    if(header[0]==4)
        tryGetData();
}

void P2PClient::tryGetData()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(header[0]!=4)
    {
        std::cerr << "P2PClient::tryGetData(): header[0]!=4" << std::endl;
        abort();
    }
    #endif
    uint32_t sizeToGet=0;
    memcpy(&sizeToGet,header+1,4);
    //drop empty message
    if(sizeToGet==0)
    {
        header[0]=0;
        return;
    }
    //process by incremental block
    ssize_t size=0;
    do
    {
        size_t sizeToRead=sizeof(readBuffer);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(data.size()>sizeToGet)
        {
            std::cerr << "P2PClient::tryGetData(): data.size()>sizeToGet: " << data.size() << "," << sizeToGet << std::endl;
            abort();
        }
        #endif
        const size_t sizeRemaining=sizeToGet-data.size();
        if(sizeToRead>sizeRemaining)
            sizeToRead=sizeRemaining;
        size=read(readBuffer,sizeToRead);
        if(size>0)
        {
            data.insert(data.size(), std::string(readBuffer,size));
            std::cerr << "P2PClient::tryGetData(): data grow of: " << size << " to be now " << data.size() << std::endl;
        }
    } while(size>0 && data.size()<sizeToGet);
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(data.size()>sizeToGet)
    {
        std::cerr << "P2PClient::tryGetData(): data.size()>sizeToGet: " << data.size() << "," << sizeToGet << std::endl;
        abort();
    }
    #endif
    if(data.size()==sizeToGet)
    {
        std::cerr << "P2PClient::tryGetData(): message: " << data << std::endl;

        //reset the message var
        data.clear();
        header[0]=0;
        return;
    }
}

void P2PClient::close()
{
    if(infd!=-1)
    {
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        Epoll::epoll.ctl(EPOLL_CTL_DEL, infd, NULL);
        ::close(infd);
        //std::cout << "Closed connection on descriptor" << infd << std::endl;
        infd=-1;
    }
}

ssize_t P2PClient::read(char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    return ::read(infd, buffer, bufferSize);
}

ssize_t P2PClient::write(const char *buffer,const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
    const int &size=::write(infd, buffer, bufferSize);
    if(size != (ssize_t)bufferSize)
    {
        if(errno != EAGAIN)
        {
            std::cerr << "Write socket error: " << errno << std::endl;
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

bool P2PClient::isValid() const
{
    return infd!=-1;
}

long int P2PClient::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    unsigned long int nbytes=0;
    // gives shorter than true amounts on Unix domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
        return nbytes;
    else
        return -1;
}

int P2PClient::getinfd()
{
    return infd;
}
