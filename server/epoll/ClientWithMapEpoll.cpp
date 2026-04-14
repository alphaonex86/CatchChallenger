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
    //slot is being reused for a fresh connection (Free -> None): drop any
    //leftover DDOS counters from the previous tenant of this slot, otherwise
    //the new client inherits stale data and DdosBuffer::total() may falsely
    //report the ">300s without flush" guard.
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    doDDOSReset();
    #endif
    //reset visibility state so PATH1 (full insert) runs on first map tick
    sendedMap=65535;
    sendedStatus.clear();
    stat=ClientStat::None;
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
