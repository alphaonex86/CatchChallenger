
#include "EventLoopClient.hpp"
#include "win32_compat.hpp"

#include <iostream>
#ifndef _WIN32
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>   //recv() + MSG_DONTWAIT
#include <fcntl.h>
#endif
#include <cstring>
#include <errno.h>
#include <string.h>
#include "EventLoop.hpp"

using namespace CatchChallenger;

EventLoopClient::EventLoopClient(const int &infd) :
#ifdef CATCHCHALLENGER_IO_URING
    uringOutstanding(0),
#endif
    infd(infd)
{
//    std::cerr << "EventLoopClient::EventLoopClient infd: " << infd << std::endl;
}

EventLoopClient::~EventLoopClient()
{
    close();
}

void EventLoopClient::reopen(const int &infd)
{
//    std::cerr << "EventLoopClient::reopen infd: " << infd << std::endl;
    close();
    this->infd=infd;
}

void EventLoopClient::close()
{
    //std::cerr << "EventLoopClient::close infd: " << infd << std::endl;
    if(infd!=-1)
    {
        char tempBuffer[4096];
        //purge the buffer to prevent multiple EPOLLIN due to level trigger (vs edge trigger)
        while(read(tempBuffer,sizeof(tempBuffer))>0)
        {}
        /* Closing the descriptor will make epoll remove it
        from the set of descriptors which are monitored. */
        EventLoop::loop.ctl(EPOLL_CTL_DEL, infd, NULL);
        cc_close_socket(infd);
        //std::cout << "Closed connection on descriptor " << infd << std::endl;
        infd=-1;
    }
}

void EventLoopClient::closeSocket()
{
    close();
}

bool EventLoopClient::socketIsOpen()
{
    return infd!=-1;
}

bool EventLoopClient::socketIsClosed()
{
    return !socketIsOpen();
}

ssize_t EventLoopClient::read(char *buffer,const size_t &bufferSize)
{
    //std::cerr << "EventLoopClient::read infd: " << infd << std::endl;
    if(infd==-1)
        return -1;
#if !defined(_WIN32) && !defined(__DJGPP__) && !defined(CC_TARGET_ESP32)
    //POSIX: ONE recv() per call, and nothing else.
    //
    //The accepted client socket is deliberately left BLOCKING
    //(main-unix.cpp: make_non_blocking() is disabled for it, because a
    //non-blocking socket short-writes the tens-of-KB datapack packets and
    //this backend has no output queue to finish them). A blocking socket is
    //why the old path asked the kernel twice before every read: once
    //ioctl(FIONREAD) to learn how many bytes could be taken without
    //blocking, and once fcntl(F_GETFL) to re-check the O_NONBLOCK flag.
    //Both were pure overhead: FIONREAD only sized a read the kernel already
    //bounds by itself, and the flag is a constant (never F_SETFL'd on this
    //fd after accept -- verified against every fcntl call site).
    //
    //MSG_DONTWAIT gives per-CALL non-blocking semantics without touching the
    //socket flag, so the write path keeps its blocking full-send guarantee
    //untouched, and the read can no longer stall the single-threaded event
    //loop. Same bytes, same order: recv() on a stream socket returns exactly
    //what read() would. Same 3 outcomes the caller already handles --
    //>0 bytes, 0 = EOF, EAGAIN = drained (reported as 0, exactly as the
    //FIONREAD==0 case did). The caller (parseIncommingData) loops until it
    //gets <=0, which now means "read until EAGAIN": the contract EPOLLET
    //requires (main-unix.cpp registers client fds with EPOLLET).
    //
    //The hard-error path tears the socket down INLINE instead of calling
    //close(): close() drains through this same read() and would re-enter the
    //error branch forever (the recursion the ESP32 branch below documents).
    {
        const ssize_t count=::recv(infd,buffer,bufferSize,MSG_DONTWAIT);
        if(count<0)
        {
            if(errno==EAGAIN || errno==EWOULDBLOCK)
                return 0;//socket drained, nothing pending right now
            std::cerr << "Read socket error, errno: " << errno << std::endl;
            EventLoop::loop.ctl(EPOLL_CTL_DEL, infd, NULL);
            cc_close_socket(infd);
            infd=-1;//isValid()==false -> the main loop drops the client
            return -1;
        }
        return count;//0 = peer closed; EPOLLRDHUP/HUP removes the client
    }
#else
    const int64_t bytesAvailableVar=bytesAvailable();
    //need more performance? change the API for 0 copy API
    if(bytesAvailableVar<=0)//non blocking for read
    {
#ifdef _WIN32
        //Windows sockets are always set non-blocking via ioctlsocket()
        //in SocketUtil. Just call recv() and let WSAEWOULDBLOCK surface.
        const int r=::recv(static_cast<SOCKET>(infd),buffer,static_cast<int>(bufferSize),0);
        if(r==SOCKET_ERROR)
            return -1;
        return r;
#elif defined(__DJGPP__)
        //Accepted Watt-32 client sockets stay BLOCKING (make_non_blocking is
        //disabled for the client fd — see main-unix.cpp), and Watt-32 has no
        //fcntl(F_GETFL). Reads are driven entirely by bytesAvailable()
        //(FIONREAD): with nothing available, do NOT recv() here (it would block
        //the single-threaded event loop) — report "no data".
        return bytesAvailableVar;
#elif defined(CC_TARGET_ESP32)
        //lwIP's ioctl(FIONREAD) is unreliable on a freshly-readable TCP socket:
        //select() reports the fd readable while FIONREAD still answers 0, so a
        //FIONREAD-driven branch would return 0 and never drain the byte —
        //leaving the socket level-readable forever and busy-looping select()
        //(starves the FreeRTOS idle task -> Task WDT). The accepted client fd is
        //left BLOCKING (make_non_blocking disabled in main-unix), so we recv()
        //with MSG_DONTWAIT: it returns the pending bytes if any, 0 on EOF, or
        //-1/EAGAIN when genuinely empty — never blocking the single-threaded
        //event loop. ESP32-only; no other target's read path is affected.
        //A hard error (ECONNRESET/ENOTCONN/EPIPE/...) means the socket is dead
        //and lwIP keeps flagging it readable: it MUST be torn down or select()
        //busy-loops on it (Task WDT). We tear it down INLINE (DEL + close fd +
        //infd=-1) instead of calling close() — close() drains via a read() loop
        //that would re-enter here and, while FIONREAD still reports queued bytes,
        //recurse on the next hard error (stack-canary panic on the 80KB ESP32
        //stack). After this, isValid()==false so the main loop drops the client.
        //EAGAIN/EWOULDBLOCK is "nothing right now" -> 0, socket stays.
        {
            const int r=::recv(infd,buffer,bufferSize,MSG_DONTWAIT);
            if(r<0)
            {
                if(errno==EAGAIN || errno==EWOULDBLOCK)
                    return 0;//genuinely nothing to read right now
                EventLoop::loop.ctl(EPOLL_CTL_DEL, infd, NULL);
                cc_close_socket(infd);
                infd=-1;
                return -1;//isValid()==false now -> main loop drops the client
            }
            return r;//r==0 -> EOF, caller's <=0 check removes the client
        }
#endif
    }
    ssize_t count;
#ifdef _WIN32
    const int want=(bytesAvailableVar>0 && bytesAvailableVar<(ssize_t)bufferSize)
                   ?static_cast<int>(bytesAvailableVar):static_cast<int>(bufferSize);
    const int r=::recv(static_cast<SOCKET>(infd),buffer,want,0);
    count=(r==SOCKET_ERROR)?-1:r;
#elif defined(__DJGPP__)
    const int want=(bytesAvailableVar>0 && bytesAvailableVar<(ssize_t)bufferSize)
                   ?static_cast<int>(bytesAvailableVar):static_cast<int>(bufferSize);
    const int r=::recv(infd,buffer,want,0);
    count=(r<0)?-1:r;
#else
    //CC_TARGET_ESP32 only (POSIX takes the recv(MSG_DONTWAIT) path above):
    //lwIP's read() is the one that must stay sized by FIONREAD.
    if(bytesAvailableVar>0 && bytesAvailableVar<(ssize_t)bufferSize)
        count=::read(infd, buffer, bytesAvailableVar);
    else
        count=::read(infd, buffer, bufferSize);
#endif
    if(count == -1)
    {
        /* If errno == EAGAIN, that means we have read all
        data. So go back to the main loop. */
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
            std::cerr << "Read socket error" << std::endl;
            close();
            return -1;
        }
    }
    //std::cerr << "EventLoopClient::read infd: " << infd << ", count: " << count << std::endl;
    return count;
#endif
}

#ifdef CATCHCHALLENGER_IO_URING
void EventLoopClient::uringOpStarted()
{
    uringOutstanding++;
}

void EventLoopClient::uringOpFinished()
{
    if(uringOutstanding>0)
        uringOutstanding--;
    //socket idle again: whatever piled up behind the in-flight op goes now
    if(uringOutstanding==0 && !asyncSendPending.empty() && infd!=-1)
    {
        asyncSendBuf.swap(asyncSendPending);
        asyncSendPending.clear();
        submitAsyncSendBuf();
    }
}

bool EventLoopClient::submitAsyncSendBuf()
{
    if(asyncSendBuf.empty())
        return false;
    if(!EventLoop::loop.submitAsyncSend(infd,asyncSendBuf.data(),
                                        static_cast<unsigned int>(asyncSendBuf.size()),
                                        static_cast<BaseClassSwitch *>(this)))
        return false;
    uringOpStarted();
    return true;
}

void EventLoopClient::onAsyncSendDone(int res)
{
    if(res<0)
    {
        //socket errored: drop everything, the disconnect path takes over
        asyncSendBuf.clear();
        asyncSendPending.clear();
    }
    else
    {
        const size_t sent=static_cast<size_t>(res);
        if(sent<asyncSendBuf.size())
        {
            //short write: the tail MUST precede anything queued meanwhile,
            //otherwise the stream reorders. Rebuild in order and hand it to
            //uringOpFinished(), which runs right after us and resubmits.
            asyncSendBuf.erase(asyncSendBuf.begin(),asyncSendBuf.begin()+sent);
            if(!asyncSendPending.empty())
            {
                asyncSendBuf.insert(asyncSendBuf.end(),
                                    asyncSendPending.begin(),asyncSendPending.end());
                asyncSendPending.clear();
            }
            asyncSendPending.swap(asyncSendBuf);
        }
        else
            asyncSendBuf.clear();
    }
}
#endif

ssize_t EventLoopClient::write(const char *buffer, const size_t &bufferSize)
{
    if(infd==-1)
        return -1;
#ifdef CATCHCHALLENGER_IO_URING
    if(bufferSize>0)
    {
        if(uringOutstanding>0)
        {
            //another op owns this fd (an ordinary send, or the datapack
            //chain): queue. Writing past it would reorder the stream.
            asyncSendPending.insert(asyncSendPending.end(),buffer,buffer+bufferSize);
            return static_cast<ssize_t>(bufferSize);
        }
        //copy: the caller's buffer is the shared output scratch, overwritten by
        //the next client of this same tick while the SQE still reads it
        asyncSendBuf.assign(buffer,buffer+bufferSize);
        if(submitAsyncSendBuf())
            return static_cast<ssize_t>(bufferSize);
        //SQ ring full and nothing in flight here: a synchronous write is safe
        asyncSendBuf.clear();
    }
#endif
#ifdef _WIN32
    const int sret=::send(static_cast<SOCKET>(infd),buffer,static_cast<int>(bufferSize),0);
    const ssize_t size=(sret==SOCKET_ERROR)?-1:sret;
#elif defined(__DJGPP__)
    //Watt-32 has no write() on sockets, and its send() does NOT reliably queue
    //a buffer larger than its internal TCP window in a single call — it can
    //report the full count while only part of the bytes are actually
    //transmitted (datapack packets are tens of KB, well past the window). Send
    //in bounded chunks, looping until the whole block is out. The socket is
    //blocking, so each send() blocks until its chunk is accepted.
    ssize_t size=0;
    while(static_cast<size_t>(size)<bufferSize)
    {
        const size_t remain=bufferSize-static_cast<size_t>(size);
        const int chunk=(remain>4096)?4096:static_cast<int>(remain);
        const int sret=::send(infd,buffer+size,chunk,0);
        if(sret<0)
        {
            if(errno==EWOULDBLOCK)
                continue;//blocking socket: window momentarily full, retry
            size=-1;
            break;
        }
        size+=sret;
    }
#else
    const ssize_t size = ::write(infd, buffer, bufferSize);
#endif
    if(size != (ssize_t)bufferSize)
    {
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
            if(errno!=104)
                std::cerr << "Write socket error, errno: " << errno << std::endl;
            else
                std::cerr << "Write socket error, Connection reset by peer (errno: 104)" << std::endl;
            close();
            return -1;
        }
        else
        {
            std::cerr << "Write socket full: EAGAIN for size:" << bufferSize << std::endl;
            return size;
        }
    }
    else
        return size;
}

BaseClassSwitch::EventLoopObjectType EventLoopClient::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Client;
}

#ifdef CATCHCHALLENGER_IO_URING
int EventLoopClient::recvMultishotFd() const
{
    //Return -1 once the socket has been closed: re-arming against
    //a stale fd would either fail with -EBADF (best case) or arm
    //against a recycled fd belonging to a different client.
    return infd;
}
#endif

bool EventLoopClient::isValid() const
{
    return infd!=-1;
}

long int EventLoopClient::bytesAvailable() const
{
    if(infd==-1)
        return -1;
    long int nbytes=0;
#ifdef _WIN32
    u_long avail=0;
    if(::ioctlsocket(static_cast<SOCKET>(infd),FIONREAD,&avail)==0)
    {
        nbytes=static_cast<long int>(avail);
#elif defined(__DJGPP__)
    //Watt-32: plain ioctl() doesn't drive sockets; use ioctlsocket(FIONREAD).
    int avail=0;
    if(::ioctlsocket(infd,FIONREAD,&avail)==0)
    {
        nbytes=static_cast<long int>(avail);
#else
    // gives shorter than true amounts on EventLoop domain sockets.
    if(ioctl(infd, FIONREAD, &nbytes)>=0)
    {
#endif
        if(nbytes<0 || nbytes>1024*1024*1024)
        {
            /*if(errno!=11)
                std::cerr << "ioctl(infd, FIONREAD, &nbytes) return incorrect value: " << nbytes << ", errno: " << errno << std::endl;*/
            return -1;
        }
        return nbytes;
    }
    else
        return -1;
}

