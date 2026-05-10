#ifndef CATCHCHALLENGER_CLIENTMAPMANAGEMENT_EVENT_LOOP_H
#define CATCHCHALLENGER_CLIENTMAPMANAGEMENT_EVENT_LOOP_H

#include "EventLoopClient.hpp"
#include "../base/MapManagement/ClientWithMap.hpp"

class ClientWithMapEventLoop : public CatchChallenger::EventLoopClient, public CatchChallenger::ClientWithMap
{
public:
    ClientWithMapEventLoop(const PLAYER_INDEX_FOR_CONNECTED &index_connected_player);
    void closeSocket() override;
    bool isValid() override;
    ssize_t readFromSocket(char * data, const size_t &size) override;
    ssize_t writeToSocket(const char * const data, const size_t &size) override;
    ssize_t writeFileToSocket(int file_fd, off_t *offset, size_t len) override;
    void reset(int infd);
#ifdef CATCHCHALLENGER_IO_URING
    //io_uring recv_multishot delivery: bytes are already in hand from
    //the kernel's provided-buffer-ring. Forward to the protocol parser
    //async entry. Buffer is owned by EventLoop and recycled immediately
    //after this returns — must consume synchronously.
    void onAsyncRecv(const char *buf,size_t len) override;
#endif
};

#endif // CLIENTMAPMANAGEMENT_H
