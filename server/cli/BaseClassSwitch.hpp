#ifndef BASECLASSSWITCH_H
#define BASECLASSSWITCH_H

#include <stdint.h>
#include <cstddef>

class BaseClassSwitch
{
public:
    virtual ~BaseClassSwitch() {}
#ifdef CATCHCHALLENGER_IO_URING
    //Phase 2 substrate hook: io_uring recv_multishot delivers RX bytes
    //via this method instead of the legacy epoll-event -> read() loop.
    //Default no-op; EventLoopClient overrides. Only called when the
    //buffer ring is initialised AND armRecvMultishot() was used for
    //the underlying fd. The buffer is owned by the caller and recycled
    //immediately after this returns — implementations must consume or
    //copy synchronously.
    virtual void onAsyncRecv(const char * /*buf*/,size_t /*len*/) {}
    //Phase 3 hook: a previously-submitted datapack-send SQE chain
    //has fully drained (or aborted). success==false means at least
    //one SQE in the chain returned an error and subsequent linked
    //SQEs were canceled — caller should disconnect the client since
    //bytes may have been only partially written. Default no-op.
    virtual void onAsyncSendChainComplete(bool /*success*/) {}
    //Phase 2 helper: when a recv_multishot CQE clears IORING_CQE_F_MORE
    //(the kernel dropped the persistent recv, e.g. on ENOBUFS or after
    //a hard error), EventLoop::wait() needs to re-arm. It only has the
    //user_data pointer (this), not the fd; subclasses with a socket
    //expose it here so the re-arm can happen automatically instead of
    //leaving RX stalled until the next caller-driven dispatch. Default
    //-1 means "no fd, don't re-arm" — keeps non-socket BaseClassSwitch
    //implementations (timers, DB handles) safe.
    virtual int recvMultishotFd() const { return -1; }
#endif
    enum EventLoopObjectType : uint8_t
    {
        #ifdef CATCHCHALLENGER_CLASS_ALLINONESERVER
            Server,
            Client,
            Timer,
            Database,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            Server,
            Client,
            Timer,
            Database,
            MasterLink,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_LOGIN
            Server,
            Client,
            Timer,
            Database,
            MasterLink,//to have clean backtrace
            GameLink,//to have clean backtrace
        #endif
        #ifdef CATCHCHALLENGER_CLASS_MASTER
            Server,
            Client,
            Timer,
            Database,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_GATEWAY
            Server,
            Client,
            Timer,
            ClientServer,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_STATS
            Client,
            EventLoopServer,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_QT
            Database,
        #endif
        #ifdef CATCHCHALLENGER_CLASS_P2PCLUSTER
            ServerP2P,
            Timer,
            Stdin
        #endif
        #ifdef CATCHCHALLENGER_CLASS_BOT
            EventLoopServer,
            Client,
            Timer,
        #endif
    };
    virtual EventLoopObjectType getType() const = 0;
};

#endif // BASECLASSSWITCH_H
