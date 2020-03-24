#include "ClientWithSocket.hpp"

using namespace CatchChallenger;

ClientWithSocket::ClientWithSocket() :
    #ifdef EPOLLCATCHCHALLENGERSERVER
    EpollClient(-1)
    #else
    qtSocket(nullptr)
    #endif
{
}

ssize_t ClientWithSocket::read(char * data, const size_t &size)
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    return EpollClient::read(data,size);
    #else
    if(qtSocket==nullptr)
        abort();
    return qtSocket->read(data,size);//count RXSize
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
    if(qtSocket==nullptr)
        abort();
    return qtSocket->write(data,size);
    #endif
}

bool ClientWithSocket::disconnectClient()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    EpollClient::close();
    #else
    if(qtSocket==nullptr)
        abort();
    qtSocket->close();
    #endif
    Client::disconnectClient();
    return true;

}

#ifndef EPOLLCATCHCHALLENGERSERVER
void ClientWithSocket::parseIncommingData()
{
    Client::parseIncommingData();
}
#endif

void ClientWithSocket::closeSocket()
{
    disconnectClient();
}
