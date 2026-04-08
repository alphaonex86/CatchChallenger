#include "QtClientWithMap.hpp"

QtClientWithMap::QtClientWithMap(QIODevice *qtSocket, const PLAYER_INDEX_FOR_CONNECTED &index_connected_player) :
    CatchChallenger::QtClient(qtSocket),
    CatchChallenger::ClientWithMap(index_connected_player)
{
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
