#ifndef EVENT_LOOP_CLIENT_H
#define EVENT_LOOP_CLIENT_H

#include "BaseClassSwitch.hpp"
#include <sys/types.h>
#include <vector>

#define BUFFER_MAX_SIZE 4096

namespace CatchChallenger {
class EventLoopClient : public BaseClassSwitch
{
public:
    EventLoopClient(const int &infd);
    ~EventLoopClient();
    void reopen(const int &infd);
    void close();
    ssize_t read(char *buffer,const size_t &bufferSize);
    ssize_t write(const char *buffer,const size_t &bufferSize);
    #ifdef CATCHCHALLENGER_IO_URING
    virtual void onAsyncSendDone(int res) override;
    virtual void uringOpStarted() override;
    virtual void uringOpFinished() override;
    #endif
    EventLoopObjectType getType() const;
    bool isValid() const;
    long int bytesAvailable() const;

    void closeSocket();
    bool socketIsOpen();
    bool socketIsClosed();
#ifdef CATCHCHALLENGER_IO_URING
    //Expose the socket fd to EventLoop::wait() so the io_uring
    //recv_multishot reaper can re-arm via armRecvMultishot() without
    //the EventLoop having to maintain a parallel obj→fd map. Returns
    //the same value as the protected `infd` member, or -1 when the
    //socket is closed (re-arm would just bounce off -EBADF).
    int recvMultishotFd() const override;
#endif
protected:
    #ifdef CATCHCHALLENGER_IO_URING
    bool submitAsyncSendBuf();
    //bytes handed to the kernel (must outlive the SQE) and bytes waiting
    //for the socket to drain; uringOutstanding counts BOTH writers.
    std::vector<char> asyncSendBuf;
    std::vector<char> asyncSendPending;
    unsigned int uringOutstanding;
    #endif
    int infd;
};
}

#endif // EVENT_LOOP_CLIENT_H
