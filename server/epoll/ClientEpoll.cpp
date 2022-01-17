#include "ClientEpoll.hpp"

using namespace CatchChallenger;

ClientEpoll::ClientEpoll() :
    EpollClient(-1)
{
}

BaseClassSwitch::EpollObjectType ClientEpoll::getType() const
{
    return BaseClassSwitch::EpollObjectType::Client;
}

ssize_t ClientEpoll::read(char * data, const size_t &size)
{
    return EpollClient::read(data,size);
}

ssize_t ClientEpoll::write(const char * const data, const size_t &size)
{
    //do some basic check on low level protocol (message split, ...)
    if(ProtocolParsingInputOutput::write(data,size)<0)
        return -1;
    return EpollClient::write(data,size);
}

bool ClientEpoll::disconnectClient()
{
    EpollClient::close();
    Client::disconnectClient();
    return true;

}

void ClientEpoll::parseIncommingData()
{
    Client::parseIncommingData();
}

void ClientEpoll::closeSocket()
{
    disconnectClient();
}
