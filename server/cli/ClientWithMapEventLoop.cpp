#include "ClientWithMapEventLoop.hpp"
#include "EventLoop.hpp"

ClientWithMapEventLoop::ClientWithMapEventLoop(const PLAYER_INDEX_FOR_CONNECTED &index_connected_player) :
    CatchChallenger::EventLoopClient(-1),
    ClientWithMap(index_connected_player)
{
}

void ClientWithMapEventLoop::reset(int infd)
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

void ClientWithMapEventLoop::closeSocket()
{
    CatchChallenger::EventLoopClient::closeSocket();
}

bool ClientWithMapEventLoop::isValid()
{
    return CatchChallenger::EventLoopClient::isValid();
}

ssize_t ClientWithMapEventLoop::readFromSocket(char * data, const size_t &size)
{
    return CatchChallenger::EventLoopClient::read(data,size);
}

ssize_t ClientWithMapEventLoop::writeToSocket(const char * const data, const size_t &size)
{
    return CatchChallenger::EventLoopClient::write(data,size);
}

ssize_t ClientWithMapEventLoop::writeFileToSocket(int file_fd, off_t *offset, size_t len)
{
    //Zero-copy datapack send. The kernel moves bytes from the file's
    //page cache directly into the socket's send queue; no userspace
    //buffer at all. Same on every backend (unix/poll/io_uring) — see
    //EventLoop::sendFile for the rationale.
    if(this->infd==-1)
        return -1;
    return EventLoop::sendFile(this->infd,file_fd,offset,len);
}
