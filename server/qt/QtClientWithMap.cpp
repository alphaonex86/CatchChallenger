#include "QtClientWithMap.hpp"

QtClientWithMap::QtClientWithMap(QIODevice *qtSocket, const PLAYER_INDEX_FOR_CONNECTED &index_connected_player) :
    CatchChallenger::QtClient(qtSocket),
    CatchChallenger::ClientWithMap(index_connected_player)
{
    //Mirror ClientWithMapEpoll::reset(infd): transition Free -> None so the
    //0xA0 protocol-header packet is accepted by ClientNetworkRead. In the epoll
    //server this happens when a pre-allocated slot is reused; in the Qt server
    //the client is freshly heap-allocated per connection so the transition has
    //to be performed here, otherwise Client::setToDefault() leaves stat==Free
    //and the server kicks the client on 0xA0 ("stat!=EpollClientLoginStat::None").
    stat=ClientStat::None;
}

void QtClientWithMap::parseIncommingData()
{
    Client::parseIncommingData();
}

void QtClientWithMap::closeSocket()
{
    CatchChallenger::QtClient::closeSocket();
}

bool QtClientWithMap::isValid()
{
    return CatchChallenger::QtClient::isValid();
}

ssize_t QtClientWithMap::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::QtClient::read(data,size);
}

ssize_t QtClientWithMap::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::QtClient::write(data,size);
}
