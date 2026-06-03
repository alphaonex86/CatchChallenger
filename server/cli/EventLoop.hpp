#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

// IO backend façade. Default is real epoll. Define one of:
//   CATCHCHALLENGER_SELECT   -> use select(2), most portable; FD_SETSIZE-bound
//   CATCHCHALLENGER_POLL     -> use poll(2), portable old-school path
//   CATCHCHALLENGER_IO_URING -> use io_uring (kernel >=6.0 preferred, liburing 2.4+)
//                               with IORING_SETUP_SINGLE_ISSUER + DEFER_TASKRUN
//                               (auto-fallback to plain init on older kernels,
//                               which still works since 5.13).
// to switch backend. Public API stays the same on purpose: existing
// call sites pass epoll_event and EPOLL* mask bits regardless of
// backend. The non-epoll backends translate internally.
#include "win32_compat.hpp"
#ifdef CATCHCHALLENGER_IO_URING
#include <string>
#include <vector>
#endif

#if (defined(CATCHCHALLENGER_SELECT) + defined(CATCHCHALLENGER_POLL) + defined(CATCHCHALLENGER_IO_URING)) > 1
#error CATCHCHALLENGER_SELECT, CATCHCHALLENGER_POLL, and CATCHCHALLENGER_IO_URING are mutually exclusive
#endif

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
    bool init();
    int wait(epoll_event *events,const int &maxevents);
    int ctl(int __op, int __fd,epoll_event *__event);
    static EventLoop loop;

#if defined(__DJGPP__) || defined(CC_TARGET_ESP32)
    //MS-DOS and ESP32 (lwIP/FreeRTOS) have no timerfd and no threads, so periodic
    //EventLoopTimer ticks cannot come from a readable fd. Instead the timer
    //registers its schedule here; wait() uses select(2)'s timeout to sleep until
    //the soonest one is due and then returns a synthetic EPOLLIN event carrying
    //`timer` (the main loop dispatches it to EventLoopTimer::exec()). offset is
    //the delay to the first tick (0 == msec); msec is the repeat interval;
    //singleShot fires once.
    void dosTimerArm(void *timer,unsigned int msec,unsigned int offset,bool singleShot);
    void dosTimerDisarm(void *timer);
#endif

    //Zero-copy file send: writes len bytes from in_fd (starting at *offset)
    //into the socket sock_fd. *offset is updated. Returns number of bytes
    //sent (>=0) or -1 on error. The default (epoll) and CATCHCHALLENGER_POLL
    //backends use sendfile(2). The CATCHCHALLENGER_IO_URING backend
    //submits a splice SQE pair through a per-engine pipe and returns
    //the synchronously-known short-write result; longer transfers are
    //driven asynchronously by completion handling.
    static ssize_t sendFile(int sock_fd,int in_fd,off_t *offset,size_t len);

#ifdef CATCHCHALLENGER_IO_URING
    //Phase 2 substrate: arm a recv_multishot SQE with provided-buffer ring
    //bgid=0 against fd. user_data must point to a BaseClassSwitch* whose
    //onAsyncRecv(buf,len) override consumes incoming bytes. Returns true
    //on submit success. Available only when buffer ring init succeeded
    //(check multishotEnabled()). Currently NOT auto-armed by the accept
    //path: that wiring is staged for a follow-up commit so the existing
    //poll_multishot dispatch stays primary while the substrate matures.
    bool armRecvMultishot(int fd,void *user_data);
    bool multishotEnabled() const;
    //Phase 3: submit one SQE chain that sends a packet header, then
    //per-file (metadata bytes + splice file→pipe + splice pipe→sock +
    //close file_fd). Intermediate CQEs are silently dropped; one
    //final CQE invokes client->onAsyncSendChainComplete(success).
    //Bytes referenced by header_bytes / per_file_meta MUST live until
    //completion — the caller is expected to heap-own them via
    //PendingSendOp. file_fds are closed by the chain itself; the
    //client struct must not double-close on disconnect.
    //pipe_r / pipe_w form a per-client splice pipe (lazy-created by
    //the caller, kept open for connection's life).
    //Returns false if the SQ couldn't fit the chain — caller falls
    //back to the synchronous sendfile path.
    //
    //per_file_chunks: for file i, sizes of consecutive chunks to
    //splice (sum must equal the file's intended send length). Pipe
    //capacity caps each chunk at ~64 KiB by default; caller chunks.
    bool submitDatapackChain(int sock_fd,
                             int pipe_r,int pipe_w,
                             const char *header_bytes,size_t header_len,
                             const std::vector<std::string> &per_file_meta,
                             const std::vector<int> &file_fds,
                             const std::vector<std::vector<size_t> > &per_file_chunks,
                             class BaseClassSwitch *client);
    //True iff io_uring backend is initialised (regardless of multishot).
    bool uringEnabled() const;
#endif
private:
    int efd;
};

#endif // EVENT_LOOP_H
