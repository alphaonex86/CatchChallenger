#include "ClientWithMapEpoll.hpp"

ClientWithMapEpoll::ClientWithMapEpoll(const PLAYER_INDEX_FOR_CONNECTED &index_connected_player) :
    CatchChallenger::EpollClient(-1),
    ClientWithMap(index_connected_player)
{
}

void ClientWithMapEpoll::reset(int infd)
{
    ProtocolParsingBase::reset();
    this->infd=infd;
}

void ClientWithMapEpoll::closeSocket()
{
    CatchChallenger::EpollClient::closeSocket();
}

bool ClientWithMapEpoll::isValid()
{
    return CatchChallenger::EpollClient::isValid();
}

ssize_t ClientWithMapEpoll::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EpollClient::read(data,size);
}

ssize_t ClientWithMapEpoll::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EpollClient::write(data,size);
}
