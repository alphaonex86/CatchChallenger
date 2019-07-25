#include "ClientWithSocket.h"

using namespace CatchChallenger;

ClientWithSocket::ClientWithSocket() :
    EpollClient(-1)
{
}

ssize_t ClientWithSocket::read(char * data, const size_t &size)
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    return EpollClient::read(data,size);
    #else
    ProtocolParsingInputOutput::read(data,size);//count RXSize
    #endif
}

ssize_t ClientWithSocket::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    #ifdef EPOLLCATCHCHALLENGERSERVER
    return EpollClient::write(data,size);
    #else
    #endif
}

bool ClientWithSocket::disconnectClient()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    EpollClient::close();
    return true;
    #else
    #endif
}

void ClientWithSocket::closeSocket()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    return EpollClient::closeSocket();
    #else
    #endif
}
