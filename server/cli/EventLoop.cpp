#include "EventLoop.hpp"
#include "../../general/base/GeneralVariable.hpp"

#include <iostream>
// ESP32 (lwIP) has no sendfile(2) either — same as MS-DOS (DJGPP).
#if !defined(_WIN32) && !defined(__DJGPP__) && !defined(CC_TARGET_ESP32)
#include <sys/sendfile.h>
#endif
#if !defined(_WIN32)
#include <unistd.h>
#endif

#if defined(CATCHCHALLENGER_SELECT)
    #ifndef _WIN32
    #include <sys/select.h>
    #endif
    #include <vector>
    #include <unordered_map>
#elif defined(CATCHCHALLENGER_POLL)
    #include <poll.h>
    #include <vector>
    #include <unordered_map>
#elif defined(CATCHCHALLENGER_IO_URING)
    #include <liburing.h>
    #include <vector>
    #include <unordered_map>
    #include <fcntl.h>
    #include <cerrno>
    #include <cstring>
    #include <cstdlib>
    #include <cstdio>
    #include <sys/stat.h>
    #include "BaseClassSwitch.hpp"
    #include "../../general/base/FacilityLibGeneral.hpp"
#endif

EventLoop EventLoop::loop;

#if defined(CATCHCHALLENGER_SELECT)
//----------------------------- select(2) backend ---------------------------
//Translation layer: EPOLLIN/OUT/ERR/HUP/RDHUP map to read/write/except
//fd_set membership. We keep an event-mask + user_data per registered fd
//and rebuild the three fd_sets on every wait() — select(2) destroys them
//in place when a wakeup happens, so caching is impossible. FD_SETSIZE
//is the per-process compile-time limit (1024 on glibc) — fine for the
//small server load this backend targets (i486 / Geode hardware).
namespace {
struct SelectState
{
    std::unordered_map<int,uint32_t> events;//fd -> requested EPOLL_* mask
    std::unordered_map<int,void *> ptrs;//fd -> user data
    //EPOLLOUT edge-emulation: select(2) is level-triggered, but the production
    //code registers client sockets with EPOLLET and is written for edge
    //semantics (an always-writable socket must report EPOLLOUT once, not every
    //iteration). Without this, an idle writable client makes select() return
    //immediately forever -> busy-loop (fatal on ESP32: starves the FreeRTOS
    //idle task -> Task WDT reboot; merely wasteful on DOS). We remember whether
    //we already reported writability for each fd and only re-emit EPOLLOUT on
    //the rising edge (was-not-writable -> now-writable). EPOLLIN stays
    //level-triggered (the read path drains the socket each time).
    std::unordered_map<int,bool> writableReported;//fd -> already signalled writable
};
static SelectState *g_select=nullptr;

//EOF probe for the EPOLLRDHUP synthesis (see wait()). select(2) reports a
//peer-closed socket as readable FOREVER (EOF is a level-readable condition);
//epoll instead raises EPOLLRDHUP and the main loop removes the client. recv()
//with MSG_PEEK does NOT consume data, so a live socket with pending bytes is
//untouched (>0 -> alive). recv()==0 is the unambiguous orderly-shutdown FIN —
//and ONLY 0 is treated as EOF here: a listen socket never returns 0 from recv
//(it errors), so the synthesis can never tear down the acceptor. recv()<0
//(EAGAIN, or a hard error on a RST'd CLIENT socket) is NOT handled here — the
//read path owns the error/RST teardown (see EventLoopClient::read, ESP32
//branch) so we never risk killing the listener. Only ever called after select
//reported the fd readable, so recv returns immediately and never blocks the
//single-threaded loop. SELECT-backend only — epoll/poll/io_uring unaffected.
static inline bool cc_recv_peek_is_eof(int fd)
{
    char b;
#if defined(_WIN32)
    return ::recv(static_cast<SOCKET>(fd),&b,1,MSG_PEEK)==0;
#else
    return ::recv(fd,&b,1,MSG_PEEK)==0;
#endif
}
}//namespace

#if defined(__DJGPP__) || defined(CC_TARGET_ESP32)
#include <time.h>
namespace {
//Interval-timer registry for backends with no timerfd and no threads (MS-DOS
//and ESP32/lwIP). wait() drives these off select(2)'s timeout — see
//dosTimerArm/Disarm + the wait() body.
struct DosTimer
{
    void *ptr;                  //the EventLoopTimer*, returned as event.data.ptr
    unsigned long long next_ms; //absolute time of the next tick
    unsigned int interval_ms;   //repeat period (ignored when singleShot)
    bool singleShot;
};
static std::vector<DosTimer> g_dos_timers;

//Monotonic millisecond clock.
static unsigned long long cc_now_ms()
{
#ifdef __DJGPP__
    //uclock() is DJGPP's high-resolution PIT-based counter (it reprograms the
    //8254 on first use); UCLOCKS_PER_SEC ~= 1.19 MHz.
    return static_cast<unsigned long long>(uclock())*1000ULL/UCLOCKS_PER_SEC;
#else
    //ESP32 (lwIP/newlib): monotonic since boot via clock_gettime.
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return static_cast<unsigned long long>(ts.tv_sec)*1000ULL
          +static_cast<unsigned long long>(ts.tv_nsec)/1000000ULL;
#endif
}
}//namespace

void EventLoop::dosTimerArm(void *timer,unsigned int msec,unsigned int offset,bool singleShot)
{
    if(offset==0)
        offset=msec;
    DosTimer t;
    t.ptr=timer;
    t.next_ms=cc_now_ms()+offset;
    t.interval_ms=msec;
    t.singleShot=singleShot;
    size_t i=0;
    while(i<g_dos_timers.size())
    {
        if(g_dos_timers[i].ptr==timer)
        {
            g_dos_timers[i]=t;//re-arm in place
            return;
        }
        ++i;
    }
    g_dos_timers.push_back(t);
}

void EventLoop::dosTimerDisarm(void *timer)
{
    size_t i=0;
    while(i<g_dos_timers.size())
    {
        if(g_dos_timers[i].ptr==timer)
        {
            g_dos_timers.erase(g_dos_timers.begin()+i);
            return;
        }
        ++i;
    }
}
#endif
#elif defined(CATCHCHALLENGER_POLL)
//----------------------------- poll(2) backend -----------------------------
//Translation layer: EPOLLIN/OUT/ERR/HUP/RDHUP and POLLIN/OUT/ERR/HUP/RDHUP
//are different bit values, so we map both ways. We keep one pollfd entry
//per registered fd and a parallel ptrs_ map fd->user_data.
namespace {
struct PollState
{
    std::vector<pollfd> pfds;
    std::vector<void *> ptrs;//1:1 with pfds
    std::unordered_map<int,size_t> idx;//fd -> index in pfds
};
static PollState *g_poll=nullptr;

short unixToPoll(uint32_t e)
{
    short r=0;
    if(e & EPOLLIN)    r|=POLLIN;
    if(e & EPOLLOUT)   r|=POLLOUT;
    if(e & EPOLLRDHUP) r|=POLLRDHUP;
    //EPOLLERR/EPOLLHUP are reported by the kernel regardless of request
    return r;
}
uint32_t pollToEpoll(short r)
{
    uint32_t e=0;
    if(r & POLLIN)    e|=EPOLLIN;
    if(r & POLLOUT)   e|=EPOLLOUT;
    if(r & POLLERR)   e|=EPOLLERR;
    if(r & POLLHUP)   e|=EPOLLHUP;
    if(r & POLLRDHUP) e|=EPOLLRDHUP;
    if(r & POLLNVAL)  e|=EPOLLERR;
    return e;
}
}//namespace
#elif defined(CATCHCHALLENGER_IO_URING)
//----------------------------- io_uring backend ----------------------------
//Each registered fd is submitted as a poll-multishot SQE. Its user_data
//is the same opaque pointer epoll users supplied. CQEs are translated
//to epoll_event entries. ctl(MOD)/ctl(DEL) post a poll_remove SQE.
//
//Phase 2 substrate (recv_multishot + provided buffer ring): when the
//kernel supports it we additionally set up a 4096-entry buffer ring
//(bgid=0, 4 KiB per buffer = 16 MiB total). armRecvMultishot() submits
//a recv_multishot SQE whose user_data pointer has the low bit ORed to 1
//to flag "this CQE carries received bytes from the buffer ring". The
//wait() loop recognises the tag, fetches the buffer, dispatches to the
//owning EventLoopClient via BaseClassSwitch::onAsyncRecv(), then recycles
//the buffer back into the ring. If the kernel doesn't support buf rings
//(<5.19) we just skip the setup and the existing poll_multishot path
//remains the only RX route. Pointer alignment guarantees bit 0 is free.
#include <poll.h>
namespace {
struct UringState
{
    io_uring ring;
    std::unordered_map<int,void *> ptrs;//fd -> user data
    //Phase 2 buffer ring (provided buffers, bgid=0). nullptr when setup
    //failed or kernel is too old.
    io_uring_buf_ring *buf_ring;
    void *buf_storage;//page-aligned backing store for all buffers
    unsigned int buf_count;//ring entries (power of two)
    unsigned int buf_size;//bytes per buffer
    bool multishot_enabled;
    unsigned long enobufs_count;//rate-limited log counter
    unsigned long rearm_fail_count;//armRecvMultishot retry rate-limit
};
static UringState *g_uring=nullptr;

//Tagging scheme on cqe->user_data (low 2 bits, mutually exclusive):
// 00 -> poll_multishot CQE (legacy path), pointer is BaseClassSwitch*
// 01 -> recv_multishot CQE, pointer = (BaseClassSwitch*)(udata & ~3)
// 10 -> Phase 3 send-chain final CQE, pointer = (PendingSendOp*)(udata & ~3)
//Pointer alignment makes low 2 bits always 0 for legitimate heap/stack
//object addresses (malloc returns 16-byte-aligned on glibc).
static inline __u64 URING_TAG_RECV(void *p)
{
    return reinterpret_cast<__u64>(p) | 0x1ULL;
}
static inline bool URING_IS_RECV(__u64 u)
{
    return (u & 0x3ULL) == 0x1ULL;
}
static inline __u64 URING_TAG_SEND_CHAIN(void *p)
{
    return reinterpret_cast<__u64>(p) | 0x2ULL;
}
static inline bool URING_IS_SEND_CHAIN(__u64 u)
{
    return (u & 0x3ULL) == 0x2ULL;
}
static inline void *URING_UNTAG(__u64 u)
{
    return reinterpret_cast<void *>(u & ~0x3ULL);
}

//Phase 3: per-call state for an in-flight datapack-send SQE chain.
//The chain submits header_send + per-file (metadata_send +
//splice file→pipe + splice pipe→sock + close file_fd). All
//intermediate SQEs use user_data=0 (dropped silently); only the
//final SQE in the chain carries URING_TAG_SEND_CHAIN(this) so we
//get exactly one completion CQE per chain.
//
//Owned bytes (header / per-file metadata) live here because the
//SQE references the buffer by pointer; the chain takes time to
//drain so the bytes must outlive Client::sendFileContent() return.
struct PendingSendOp
{
    BaseClassSwitch *client;       //onAsyncSendChainComplete() target
    std::vector<int> file_fds;     //all pre-opened files; closed by chain
    std::string header_buf;        //packet header bytes (6 = 1+4+1)
    std::vector<std::string> meta; //per-file metadata bytes (1+name+4 each)
};

uint32_t pollMaskToEpoll(uint32_t mask)
{
    uint32_t e=0;
    if(mask & POLLIN)    e|=EPOLLIN;
    if(mask & POLLOUT)   e|=EPOLLOUT;
    if(mask & POLLERR)   e|=EPOLLERR;
    if(mask & POLLHUP)   e|=EPOLLHUP;
    if(mask & POLLRDHUP) e|=EPOLLRDHUP;
    if(mask & POLLNVAL)  e|=EPOLLERR;
    return e;
}
short unixMaskToPoll(uint32_t e)
{
    short r=0;
    if(e & EPOLLIN)    r|=POLLIN;
    if(e & EPOLLOUT)   r|=POLLOUT;
    if(e & EPOLLRDHUP) r|=POLLRDHUP;
    return r;
}
}//namespace
#endif

EventLoop::EventLoop()
{
    efd=-1;
}

EventLoop::~EventLoop()
{
#if defined(CATCHCHALLENGER_SELECT)
    delete g_select;
    g_select=nullptr;
#elif defined(CATCHCHALLENGER_POLL)
    delete g_poll;
    g_poll=nullptr;
#elif defined(CATCHCHALLENGER_IO_URING)
    if(g_uring!=nullptr)
    {
#ifdef CC_HAS_URING_BUF_RING
        if(g_uring->buf_ring!=nullptr)
        {
            io_uring_free_buf_ring(&g_uring->ring,g_uring->buf_ring,
                                   g_uring->buf_count,0);
            g_uring->buf_ring=nullptr;
        }
#endif
        if(g_uring->buf_storage!=nullptr)
        {
            free(g_uring->buf_storage);
            g_uring->buf_storage=nullptr;
        }
        io_uring_queue_exit(&g_uring->ring);
        delete g_uring;
        g_uring=nullptr;
    }
#else
    if(efd!=-1)
    {
        ::close(efd);
        efd=-1;
    }
#endif
}

#if defined(CATCHCHALLENGER_IO_URING)
// Dedicated io_uring instance for whole-FILE reads (datapack load on boot,
// datapack files streamed to clients on sync). Deliberately separate from the
// socket ring: an IORING_SETUP_IOPOLL ring (the CATCHCHALLENGER_IO_URING_IOPOLL
// opt-in) accepts ONLY pollable O_DIRECT block I/O and would reject the socket
// poll_multishot/recv/send the main ring runs. Lazily created on first read,
// reused for the process lifetime, and — like everything else here — only ever
// submitted to from the single event-loop thread.
static struct io_uring cc_file_ring;
static bool cc_file_ring_ready=false;
// IOPOLL needs genuine O_DIRECT. Where the libc exposes O_DIRECT as 0 (the
// platform has no direct-I/O), a polled ring has nothing pollable to complete,
// so compile the file ring as a plain (interrupt-driven) ring instead.
#if defined(CATCHCHALLENGER_IO_URING_IOPOLL) && (O_DIRECT != 0)
#define CC_FILE_RING_IOPOLL 1
#else
#define CC_FILE_RING_IOPOLL 0
#endif

static bool cc_ensure_file_ring()
{
    if(cc_file_ring_ready)
        return true;
    struct io_uring_params p;
    memset(&p,0,sizeof(p));
#if CC_FILE_RING_IOPOLL
    p.flags|=IORING_SETUP_IOPOLL;
#endif
    int r=io_uring_queue_init_params(64,&cc_file_ring,&p);
#if CC_FILE_RING_IOPOLL
    if(r<0)
    {
        //kernel/fs without IOPOLL support -> fall back to a plain ring so the
        //file reads still go through io_uring (just interrupt-driven).
        memset(&p,0,sizeof(p));
        r=io_uring_queue_init(64,&cc_file_ring,0);
    }
#endif
    if(r<0)
        return false;
    cc_file_ring_ready=true;
    return true;
}

// Read a whole file via the file ring. Returns false on ANY problem so the
// caller (FacilityLibGeneral::readWholeFile) transparently falls back to a
// blocking fopen/fread — correctness never depends on the ring succeeding.
// Signature matches FacilityLibGeneral::WholeFileRingReader.
static bool cc_uring_read_whole_file(const std::string &path,std::vector<char> &out)
{
    if(!cc_ensure_file_ring())
        return false;
    int openFlags=O_RDONLY;
#if CC_FILE_RING_IOPOLL
    //IOPOLL completions require the fd to bypass the page cache (O_DIRECT).
    bool useDirect=true;
    openFlags|=O_DIRECT;
#else
    bool useDirect=false;
#endif
    int fd=::open(path.c_str(),openFlags);
#if CC_FILE_RING_IOPOLL
    if(fd<0 && (errno==EINVAL || errno==ENOTSUP))
    {
        //e.g. tmpfs rejects O_DIRECT. A buffered fd can't be polled by an
        //IOPOLL ring, so the read below will fail and the caller drops to the
        //blocking path — fine, tmpfs is the RAM-DB bench case where io_uring
        //file I/O buys nothing anyway.
        useDirect=false;
        fd=::open(path.c_str(),O_RDONLY);
    }
#endif
    if(fd<0)
        return false;
    struct stat st;
    if(fstat(fd,&st)!=0 || st.st_size<0)
    {
        ::close(fd);
        return false;
    }
    const size_t fileSize=static_cast<size_t>(st.st_size);
    if(fileSize==0)
    {
        ::close(fd);
        out.clear();
        return true;
    }
    void *buf=nullptr;
    size_t readLen=fileSize;
    bool ownBuf=false;
    if(useDirect)
    {
        //O_DIRECT demands an aligned buffer and a block-multiple length; the
        //kernel returns the real byte count (<= fileSize) at EOF.
        const size_t ALIGN=4096;
        readLen=((fileSize+ALIGN-1)/ALIGN)*ALIGN;
        if(posix_memalign(&buf,ALIGN,readLen)!=0)
        {
            ::close(fd);
            return false;
        }
        ownBuf=true;
    }
    else
    {
        out.resize(fileSize);
        buf=out.data();
    }
    struct io_uring_sqe *sqe=io_uring_get_sqe(&cc_file_ring);
    if(sqe==nullptr)
    {
        if(ownBuf)
            free(buf);
        ::close(fd);
        return false;
    }
    io_uring_prep_read(sqe,fd,buf,static_cast<unsigned>(readLen),0);
    if(io_uring_submit_and_wait(&cc_file_ring,1)<0)
    {
        if(ownBuf)
            free(buf);
        ::close(fd);
        return false;
    }
    struct io_uring_cqe *cqe=nullptr;
    if(io_uring_wait_cqe(&cc_file_ring,&cqe)<0 || cqe==nullptr)
    {
        if(ownBuf)
            free(buf);
        ::close(fd);
        return false;
    }
    const int res=cqe->res;
    io_uring_cqe_seen(&cc_file_ring,cqe);
    ::close(fd);
    if(res<0)
    {
        if(ownBuf)
            free(buf);
        return false;
    }
    const size_t got=static_cast<size_t>(res);
    if(useDirect)
    {
        const size_t copyLen=(got<fileSize)?got:fileSize;
        out.assign(static_cast<char *>(buf),static_cast<char *>(buf)+copyLen);
        free(buf);
    }
    else
    {
        if(got<fileSize)
            out.resize(got);
    }
    return true;
}

// Whole-file WRITE for FILE_DB saves (character/account/server/clan), routed
// through cc_file_ring — the SAME ring as reads (O_DIRECT when the IOPOLL
// opt-in is on, else buffered). CRASH-SAFE via write-to-temp + atomic rename:
// the bytes are written to a sibling "<dir>/.<name>.tmp", any O_DIRECT block
// padding is trimmed on the TEMP with ftruncate (never the target), then the
// temp is rename(2)'d over the target. A kill/crash anywhere leaves either the
// OLD target intact or the new complete one — never the half-written / padded
// state that previously corrupted reloads. The leading '.' keeps the temp out
// of the database/server/clans readdir scan (LocalClientHandlerClan skips
// '.'-prefixed entries). Returns false on ANY problem so the caller falls back
// to a blocking rewrite. No fsync: gives atomicity (no torn file), matching the
// previous ofstream-close durability rather than power-loss durability.
// Signature matches FacilityLibGeneral::WholeFileRingWriter.
static bool cc_uring_write_whole_file(const std::string &path,const char *data,size_t size)
{
    if(!cc_ensure_file_ring())
        return false;
    //temp = "<dir>/.<basename>.tmp" so directory scans (which skip '.'-prefixed
    //names) never see the in-flight file.
    std::string tmp;
    const std::string::size_type slash=path.rfind('/');
    if(slash==std::string::npos)
        tmp="."+path+".tmp";
    else
        tmp=path.substr(0,slash+1)+"."+path.substr(slash+1)+".tmp";
    int openFlags=O_WRONLY|O_CREAT|O_TRUNC;
#if CC_FILE_RING_IOPOLL
    bool useDirect=true;
    openFlags|=O_DIRECT;
#else
    bool useDirect=false;
#endif
    int fd=::open(tmp.c_str(),openFlags,0644);
#if CC_FILE_RING_IOPOLL
    if(fd<0 && (errno==EINVAL || errno==ENOTSUP))
    {
        //filesystem rejects O_DIRECT (tmpfs): a buffered fd can't be polled by
        //an IOPOLL ring, so the write below fails and the caller drops to the
        //blocking path. Acceptable (tmpfs is the RAM-DB case).
        useDirect=false;
        fd=::open(tmp.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);
    }
#endif
    if(fd<0)
        return false;
    bool ok=true;
    if(size>0)
    {
        void *buf=nullptr;
        size_t writeLen=size;
        bool ownBuf=false;
        if(useDirect)
        {
            //O_DIRECT: aligned buffer, block-multiple length (padded on the
            //TEMP only, trimmed below before the rename).
            const size_t ALIGN=4096;
            writeLen=((size+ALIGN-1)/ALIGN)*ALIGN;
            if(posix_memalign(&buf,ALIGN,writeLen)!=0)
            {
                ::close(fd);
                ::unlink(tmp.c_str());
                return false;
            }
            memcpy(buf,data,size);
            if(writeLen>size)
                memset(static_cast<char *>(buf)+size,0,writeLen-size);
            ownBuf=true;
        }
        else
            buf=const_cast<char *>(data);
        struct io_uring_sqe *sqe=io_uring_get_sqe(&cc_file_ring);
        if(sqe==nullptr)
            ok=false;
        else
        {
            io_uring_prep_write(sqe,fd,buf,static_cast<unsigned>(writeLen),0);
            if(io_uring_submit_and_wait(&cc_file_ring,1)<0)
                ok=false;
            else
            {
                struct io_uring_cqe *cqe=nullptr;
                if(io_uring_wait_cqe(&cc_file_ring,&cqe)<0 || cqe==nullptr)
                    ok=false;
                else
                {
                    const int res=cqe->res;
                    io_uring_cqe_seen(&cc_file_ring,cqe);
                    if(!(res>=0 && static_cast<size_t>(res)==writeLen))
                        ok=false;
                }
            }
        }
        if(ownBuf)
            free(buf);
        if(ok && useDirect && ftruncate(fd,static_cast<off_t>(size))!=0)
            ok=false;
    }
    ::close(fd);
    if(!ok)
    {
        ::unlink(tmp.c_str());
        return false;
    }
    //atomic publish: target is now either the old file or the new complete one.
    if(::rename(tmp.c_str(),path.c_str())!=0)
    {
        ::unlink(tmp.c_str());
        return false;
    }
    return true;
}
#endif

bool EventLoop::init()
{
#if defined(CATCHCHALLENGER_SELECT)
    g_select=new SelectState();
    efd=0;//unused but flips the "initialised" check
    return true;
#elif defined(CATCHCHALLENGER_POLL)
    g_poll=new PollState();
    efd=0;//unused but flips the "initialised" check
    return true;
#elif defined(CATCHCHALLENGER_IO_URING)
    g_uring=new UringState();
    g_uring->buf_ring=nullptr;
    g_uring->buf_storage=nullptr;
    g_uring->buf_count=0;
    g_uring->buf_size=0;
    g_uring->multishot_enabled=false;
    g_uring->enobufs_count=0;
    g_uring->rearm_fail_count=0;
    //Queue depth 4096 covers an MMORPG with hundreds of concurrent fds
    //(one poll-multishot SQE per fd plus splice/sendfile chains).
    //
    //IORING_SETUP_SINGLE_ISSUER: we promise the ring is only ever
    //submitted-to from this thread (CC's loop is strictly single-threaded).
    //Lets the kernel skip cross-thread synchronisation on the SQ side.
    //
    //IORING_SETUP_DEFER_TASKRUN: completions are processed only when
    //userspace asks for them (io_uring_wait_cqe / submit_and_wait).
    //Without it the kernel may interrupt us at arbitrary points to run
    //task work; with it, latency is more predictable and CPU stays cooler
    //on light loads. Both flags require kernel >= 6.0; we fall back to a
    //plain init on older kernels so the binary keeps working.
    struct io_uring_params params;
    memset(&params,0,sizeof(params));
    //DEFER_TASKRUN disabled: it requires every io_uring_enter() call
    //to use IORING_ENTER_GETEVENTS, which liburing's io_uring_wait_cqe
    //SHOULD set, but in practice CQEs for poll_multishot registered
    //fds don't surface in time — gsa hangs at "wait database
    //dictionary_map query" because the PG socket's CQE never arrives.
    //SINGLE_ISSUER alone is the safe win (skips kernel SQ-side lock).
    params.flags=IORING_SETUP_SINGLE_ISSUER;
    //CATCHCHALLENGER_IO_URING_SQPOLL: opt-in only — SQPOLL spins a kernel
    //thread that competes with the server thread on a single CPU core
    //(i486, Geode, Pentium III Tualatin, MIPS2 …).  Multi-core hosts only.
#ifdef CATCHCHALLENGER_IO_URING_SQPOLL
    params.flags|=IORING_SETUP_SQPOLL;
    params.sq_thread_idle=2000;
#endif
    //CATCHCHALLENGER_IO_URING_COOP_TASKRUN: kernel never signals userspace
    //to run task work; work drains only on the next io_uring_enter() call.
    //Lighter than DEFER_TASKRUN (no mandatory IORING_ENTER_GETEVENTS),
    //safe on single-core.  Requires kernel >= 5.19; falls back gracefully
    //via the existing EINVAL retry path below.
#ifdef CATCHCHALLENGER_IO_URING_COOP_TASKRUN
    params.flags|=IORING_SETUP_COOP_TASKRUN;
#endif
    //CATCHCHALLENGER_IO_URING_TASKRUN_FLAG: companion to COOP_TASKRUN —
    //kernel sets IORING_SQ_TASKRUN in sq.kflags when task work is pending,
    //so the event loop can check the flag instead of calling io_uring_enter
    //blindly.  Requires COOP_TASKRUN + kernel >= 5.19.
#ifdef CATCHCHALLENGER_IO_URING_TASKRUN_FLAG
    params.flags|=IORING_SETUP_TASKRUN_FLAG;
#endif
    //CATCHCHALLENGER_IO_URING_NO_SQARRAY: drop the SQ indirection array
    //(kernel >= 6.6).  CC's loop submits SQEs sequentially so the array
    //is never reordered; removing it saves memory and a cache line per
    //submit.  Safe on single-core, standalone flag (no companion required).
#ifdef CATCHCHALLENGER_IO_URING_NO_SQARRAY
    params.flags|=IORING_SETUP_NO_SQARRAY;
#endif
    int qret=io_uring_queue_init_params(4096,&g_uring->ring,&params);
    if(qret<0)
    {
        if(qret==-EINVAL)
        {
            std::cerr << "io_uring: SINGLE_ISSUER|DEFER_TASKRUN unsupported "
                         "(kernel < 6.0?), falling back to plain init"
                      << std::endl;
            qret=io_uring_queue_init(4096,&g_uring->ring,0);
        }
        if(qret<0)
        {
            std::cerr << "io_uring_queue_init failed: " << qret << std::endl;
            delete g_uring;
            g_uring=nullptr;
            return false;
        }
    }
    //Phase 2: try to set up the provided-buffer ring so recv_multishot
    //can land received bytes directly into pre-registered memory. If the
    //running kernel is too old (<5.19) or memory allocation fails, leave
    //multishot_enabled=false and fall back silently to the existing
    //poll_multishot RX path. NEVER fail server init over this — it's an
    //optimisation, not a requirement.
    //CC_HAS_URING_BUF_RING is set by the CMake feature check in
    //general/CCCommon.cmake; older liburing (<2.3) doesn't declare
    //io_uring_setup_buf_ring/io_uring_free_buf_ring so the whole phase
    //compiles out.
#ifdef CC_HAS_URING_BUF_RING
    const unsigned int BUF_COUNT=4096;//power of two
    const unsigned int BUF_SIZE=4096;
    {
        const size_t total=static_cast<size_t>(BUF_COUNT)*BUF_SIZE;
        void *storage=nullptr;
        const long pagesize=sysconf(_SC_PAGESIZE);
        const size_t align=(pagesize>0)?static_cast<size_t>(pagesize):4096;
        if(posix_memalign(&storage,align,total)!=0)
            storage=nullptr;
        if(storage!=nullptr)
        {
            //Zero the buffer ring so valgrind sees the bytes as
            //initialised. io_uring's recv_multishot fills these buffers
            //from kernel-space, which valgrind's memcheck can't observe;
            //a downstream read of (legitimately kernel-written) bytes
            //then triggers a "Conditional jump or move depends on
            //uninitialised value(s)" error per byte, the gateway test
            //ends up with tens of thousands of false positives, and
            //--errors-for-leak-kinds gets the process killed before any
            //real protocol exchange completes. Cost of the memset is
            //one-time at startup over 16 MiB — negligible.
            memset(storage,0,total);
            int br_err=0;
            io_uring_buf_ring *br=io_uring_setup_buf_ring(
                        &g_uring->ring,BUF_COUNT,/*bgid=*/0,/*flags=*/0,&br_err);
            if(br!=nullptr)
            {
                //Seed the ring: every buffer is initially owned by the
                //kernel. mask = BUF_COUNT-1 (power-of-two requirement).
                const int mask=io_uring_buf_ring_mask(BUF_COUNT);
                unsigned int i=0;
                while(i<BUF_COUNT)
                {
                    char *base=static_cast<char *>(storage)+
                            static_cast<size_t>(i)*BUF_SIZE;
                    io_uring_buf_ring_add(br,base,BUF_SIZE,
                                          static_cast<unsigned short>(i),
                                          mask,static_cast<int>(i));
                    ++i;
                }
                io_uring_buf_ring_advance(br,static_cast<int>(BUF_COUNT));
                g_uring->buf_ring=br;
                g_uring->buf_storage=storage;
                g_uring->buf_count=BUF_COUNT;
                g_uring->buf_size=BUF_SIZE;
                g_uring->multishot_enabled=true;
            }
            else
            {
                std::cerr << "io_uring: setup_buf_ring failed (err=" << br_err
                          << "), recv_multishot disabled" << std::endl;
                free(storage);
            }
        }
        else
        {
            std::cerr << "io_uring: posix_memalign for buf ring failed, "
                         "recv_multishot disabled" << std::endl;
        }
    }
#endif
    {
        //Phase 5: single-line backend banner so the operator sees which
        //fast paths are live at startup.
        std::cerr << "io_uring backend: SQ depth=4096, "
                     "single_issuer=" << (qret>=0?"on":"off")
                  << ", defer_taskrun=" << (qret>=0?"on":"off")
                  << ", recv_multishot="
                  << (g_uring->multishot_enabled?"available (substrate, off by default)":"off")
                  << ", async-send-chain=off (deferred)"
                  << std::endl;
    }
    //Route whole-file reads (datapack load, client datapack sync) and FILE_DB
    //whole-file writes (character/account/server saves) through the dedicated
    //file ring. Installed only for the io_uring backend; select/poll/epoll and
    //the client keep the blocking fopen/fread/fwrite path (hooks stay null).
    CatchChallenger::FacilityLibGeneral::wholeFileRingReader=&cc_uring_read_whole_file;
    CatchChallenger::FacilityLibGeneral::wholeFileRingWriter=&cc_uring_write_whole_file;
    efd=0;
    return true;
#else
    efd = epoll_create1(0);
    if(efd == -1)
    {
        std::cerr << "epoll_create failed" << std::endl;
        return false;
    }
    return true;
#endif
}

#ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
//Belt-and-suspenders against infinite kernel/user-space wakeup loops
//(missing read drain, epoll EPOLLIN never cleared, io_uring multishot
//re-armed in a tight loop, …). Counts events delivered through
//EventLoop::wait() in a sliding 1 s window; if the rate breaches
//`limit_per_second`, abort() so the test rig captures the
//backtrace via the wall-watchdog instead of the binary burning a
//full core silently.
//
//Designed for testing*.py only. Not for production: legitimate
//burst tests (testingbots, testingmulti large connexion counts,
//DDOS fuzzing in testingbyIA) MUST omit the
//CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE define at compile time.
//Threshold is fixed at 1000/s in the harness contract — change
//here AND in test/CLAUDE.md if the contract ever moves.
namespace {
struct EventRateLimiter
{
    uint64_t windowStartMs;
    unsigned int eventsInWindow;
    unsigned int limit_per_second;
};
static EventRateLimiter g_event_rate_limiter={0,0,1000};

static inline uint64_t _now_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return static_cast<uint64_t>(ts.tv_sec)*1000ULL+
           static_cast<uint64_t>(ts.tv_nsec)/1000000ULL;
}

static inline void _event_rate_account(int produced)
{
    if(produced<=0)
        return;
    const uint64_t now=_now_ms();
    if(now-g_event_rate_limiter.windowStartMs>=1000)
    {
        g_event_rate_limiter.windowStartMs=now;
        g_event_rate_limiter.eventsInWindow=0;
    }
    g_event_rate_limiter.eventsInWindow+=static_cast<unsigned int>(produced);
    if(g_event_rate_limiter.eventsInWindow>g_event_rate_limiter.limit_per_second)
    {
        std::cerr << "CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE: "
                  << g_event_rate_limiter.eventsInWindow
                  << " events in last <1s (limit "
                  << g_event_rate_limiter.limit_per_second
                  << "/s) — likely a tight wakeup loop; aborting "
                     "so the test rig can capture a backtrace"
                  << std::endl;
        std::abort();
    }
}
}
#endif

int EventLoop::wait(epoll_event *events,const int &maxevents)
{
#if defined(CATCHCHALLENGER_SELECT)
#if defined(__DJGPP__) || defined(CC_TARGET_ESP32)
    //DOS/ESP32: still service armed interval timers even when no fds are registered.
    if(g_select==nullptr)
        return 0;
    if(g_select->events.empty() && g_dos_timers.empty())
        return 0;
#else
    if(g_select==nullptr || g_select->events.empty())
        return 0;
#endif
    fd_set rfds, wfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);
    int maxfd=-1;
    for(const std::pair<const int,uint32_t> &e : g_select->events)
    {
        const int fd=e.first;
        if(fd>=FD_SETSIZE)
        {
            //select(2) silently overflows fd_set if fd >= FD_SETSIZE.
            //Surface the limit instead of corrupting memory.
            std::cerr << "EventLoop(select): fd=" << fd << " exceeds FD_SETSIZE="
                      << FD_SETSIZE << "; rebuild with the poll/unix backend"
                      << std::endl;
            return -1;
        }
        const uint32_t mask=e.second;
        if(mask & EPOLLIN)  FD_SET(fd,&rfds);
        //EPOLLOUT edge-emulation (see SelectState::writableReported): once we
        //have delivered the writable edge for this fd, STOP selecting on write
        //for it. Otherwise an always-writable client socket keeps wfds set, so
        //select() returns immediately every iteration and never blocks -> busy
        //loop (fatal on ESP32: starves the FreeRTOS idle task -> Task WDT). The
        //edge re-arms only via EPOLL_CTL_ADD/MOD (writableReported reset in
        //ctl()) — which matches epoll's EPOLLET contract for a consumer that
        //writes inline and never holds a pending send queue (as CatchChallenger
        //does). EPOLLIN stays level-triggered (the read path drains it).
        if((mask & EPOLLOUT) && !g_select->writableReported[fd]) FD_SET(fd,&wfds);
        //EPOLLERR/EPOLLHUP are reported via the exception fd_set.
        FD_SET(fd,&efds);
        if(fd>maxfd) maxfd=fd;
    }
#if defined(__DJGPP__) || defined(CC_TARGET_ESP32)
    //Sleep at most until the soonest armed timer is due — this is the DOS/ESP32
    //interval-timer source (no timerfd). With no timers, block indefinitely.
    struct timeval tv;
    struct timeval *ptv=nullptr;
    if(!g_dos_timers.empty())
    {
        const unsigned long long now=cc_now_ms();
        unsigned long long soonest=0;
        bool have=false;
        size_t ti=0;
        while(ti<g_dos_timers.size())
        {
            const unsigned long long nf=g_dos_timers[ti].next_ms;
            if(!have || nf<soonest)
            {
                soonest=nf;
                have=true;
            }
            ++ti;
        }
        const unsigned long long wait_ms=(soonest>now)?(soonest-now):0;
        tv.tv_sec=static_cast<long>(wait_ms/1000ULL);
        tv.tv_usec=static_cast<long>((wait_ms%1000ULL)*1000ULL);
        ptv=&tv;
    }
    const int n=::select(maxfd+1,&rfds,&wfds,&efds,ptv);
    //n==0 is a normal timer-timeout here, NOT an error; only n<0 with nothing
    //else to report is a real failure.
    int produced=0;
    if(n>0)
    {
        for(const std::pair<const int,uint32_t> &e : g_select->events)
        {
            if(produced>=maxevents)
                break;
            const int fd=e.first;
            uint32_t out=0;
            if(FD_ISSET(fd,&rfds))
            {
                out|=EPOLLIN;
                //EPOLLRDHUP synthesis on peer FIN (see cc_recv_peek_is_eof):
                //without it a closed-but-not-removed socket stays level-readable
                //and select() spins forever (Task WDT on ESP32). The read path
                //drains pending data first, so a 0-byte peek here is a true EOF.
                if(cc_recv_peek_is_eof(fd))
                    out|=EPOLLRDHUP|EPOLLHUP;
            }
            //EPOLLOUT edge-emulation: wfds only ever contains fds whose writable
            //edge has NOT yet been delivered (see the fd_set build above), so a
            //set wfds bit IS the rising edge. Latch it so we stop selecting on
            //write for this fd until ctl() re-arms it.
            if(FD_ISSET(fd,&wfds))
            {
                out|=EPOLLOUT;
                g_select->writableReported[fd]=true;
            }
            if(FD_ISSET(fd,&efds)) out|=EPOLLERR;
            if(out!=0)
            {
                events[produced].events=out;
                events[produced].data.ptr=g_select->ptrs[fd];
                ++produced;
            }
        }
    }
    //Append a synthetic EPOLLIN event for every timer now due, then reschedule
    //(periodic) or drop (single-shot). The main loop dispatches data.ptr to
    //EventLoopTimer::exec().
    const unsigned long long now2=cc_now_ms();
    size_t di=0;
    while(di<g_dos_timers.size())
    {
        if(produced>=maxevents)
            break;
        if(g_dos_timers[di].next_ms<=now2)
        {
            events[produced].events=EPOLLIN;
            events[produced].data.ptr=g_dos_timers[di].ptr;
            ++produced;
            if(g_dos_timers[di].singleShot)
            {
                g_dos_timers.erase(g_dos_timers.begin()+di);
                continue;//don't ++di: the next entry shifted into this slot
            }
            //schedule the next tick from completion (no catch-up storm if a
            //slow exec() overran the interval).
            g_dos_timers[di].next_ms=now2+g_dos_timers[di].interval_ms;
        }
        ++di;
    }
    #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
    _event_rate_account(produced);
    #endif
    if(n<0 && produced==0)
        return n;
    return produced;
#else
    const int n=::select(maxfd+1,&rfds,&wfds,&efds,nullptr);
    if(n<=0)
    {
        #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
        _event_rate_account(n);
        #endif
        return n;
    }
    int produced=0;
    for(const std::pair<const int,uint32_t> &e : g_select->events)
    {
        if(produced>=maxevents)
            break;
        const int fd=e.first;
        uint32_t out=0;
        if(FD_ISSET(fd,&rfds))
        {
            out|=EPOLLIN;
            //EPOLLRDHUP synthesis on peer FIN — see the DOS/ESP32 branch above
            //and cc_recv_peek_is_eof() for the full rationale.
            if(cc_recv_peek_is_eof(fd))
                out|=EPOLLRDHUP|EPOLLHUP;
        }
        //EPOLLOUT edge-emulation: wfds only ever contains fds whose writable
        //edge has NOT yet been delivered (see the fd_set build above), so a set
        //wfds bit IS the rising edge. Latch it so we stop selecting on write for
        //this fd until ctl() re-arms it.
        if(FD_ISSET(fd,&wfds))
        {
            out|=EPOLLOUT;
            g_select->writableReported[fd]=true;
        }
        if(FD_ISSET(fd,&efds)) out|=EPOLLERR;
        if(out!=0)
        {
            events[produced].events=out;
            events[produced].data.ptr=g_select->ptrs[fd];
            ++produced;
        }
    }
    #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
    _event_rate_account(produced);
    #endif
    return produced;
#endif
#elif defined(CATCHCHALLENGER_POLL)
    if(g_poll==nullptr || g_poll->pfds.empty())
        return 0;
    const int n=::poll(g_poll->pfds.data(),g_poll->pfds.size(),-1);
    if(n<=0)
    {
        #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
        _event_rate_account(n);
        #endif
        return n;
    }
    int produced=0;
    size_t i=0;
    while(i<g_poll->pfds.size() && produced<maxevents)
    {
        const short revents=g_poll->pfds[i].revents;
        if(revents!=0)
        {
            events[produced].events=pollToEpoll(revents);
            events[produced].data.ptr=g_poll->ptrs[i];
            ++produced;
        }
        ++i;
    }
    #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
    _event_rate_account(produced);
    #endif
    return produced;
#elif defined(CATCHCHALLENGER_IO_URING)
    if(g_uring==nullptr)
        return 0;
    io_uring_cqe *cqe=nullptr;
    //Block until at least one event lands.
    const int wret=io_uring_wait_cqe(&g_uring->ring,&cqe);
    if(wret<0 || cqe==nullptr)
        return -1;
    int produced=0;
    while(produced<maxevents)
    {
        //user_data==0 means this CQE is the completion of a poll_remove
        //SQE we issued ourselves (we set its data to nullptr in EPOLL_CTL_DEL
        //and EPOLL_CTL_MOD). The dispatcher would deref NULL on
        //events[i].data.ptr->getType(), so consume the CQE silently.
        //
        //cqe->res==-ECANCELED is the FINAL CQE of a poll-multishot that
        //the kernel just stopped because we asked it to (poll_remove). The
        //user already called EPOLL_CTL_DEL and may have freed the
        //associated Client object, so the original user_data ptr is no
        //longer safe to dereference. Drop these too.
        const __s32 res=cqe->res;
        const __u64 udata=cqe->user_data;
        const __u32 cqe_flags=cqe->flags;
        const bool drop=(udata==0)||(res==-ECANCELED);
        if(!drop)
        {
            if(URING_IS_RECV(udata))
            {
                //Phase 2: recv_multishot CQE. Bytes landed in a provided
                //buffer; dispatch via the BaseClassSwitch hook then recycle.
                //Do NOT surface this as a fake epoll_event to the caller;
                //the legacy dispatcher would call read() on the fd and
                //race the kernel.
                BaseClassSwitch *obj=
                    static_cast<BaseClassSwitch *>(URING_UNTAG(udata));
                if(res==-ENOBUFS)
                {
                    //Buffer ring exhausted. Rate-limit log: every 1024.
                    //The re-arm itself is handled below by rearm_eligible
                    //(which already covers res==-ENOBUFS); no flag needed.
                    if((g_uring->enobufs_count++ & 0x3FF)==0)
                        std::cerr << "io_uring recv_multishot: ENOBUFS x"
                                  << g_uring->enobufs_count << std::endl;
                }
                else if(res<0)
                {
                    //Hard error: surface as EPOLLERR via the normal path
                    //so the caller can clean up the client.
                    if(produced<maxevents)
                    {
                        events[produced].events=EPOLLERR;
                        events[produced].data.ptr=obj;
                        ++produced;
                    }
                }
                else if(res==0)
                {
                    //Peer closed. Surface as EPOLLRDHUP|EPOLLIN.
                    if(produced<maxevents)
                    {
                        events[produced].events=EPOLLRDHUP|EPOLLIN;
                        events[produced].data.ptr=obj;
                        ++produced;
                    }
                }
                else
                {
                    //Got bytes. Locate provided buffer.
                    if((cqe_flags & IORING_CQE_F_BUFFER)!=0
                       && g_uring->buf_ring!=nullptr)
                    {
                        const unsigned short bid=
                            static_cast<unsigned short>(
                                cqe_flags >> IORING_CQE_BUFFER_SHIFT);
                        char *base=static_cast<char *>(g_uring->buf_storage)+
                                   static_cast<size_t>(bid)*g_uring->buf_size;
                        obj->onAsyncRecv(base,static_cast<size_t>(res));
                        //Recycle buffer back into the ring.
                        const int mask=
                            io_uring_buf_ring_mask(g_uring->buf_count);
                        io_uring_buf_ring_add(g_uring->buf_ring,base,
                                              g_uring->buf_size,bid,mask,0);
                        io_uring_buf_ring_advance(g_uring->buf_ring,1);
                    }
                }
                //Only re-arm when the SQE actually terminated AFTER a
                //successful byte delivery (res > 0). Re-arming on
                //res==0 (peer closed) or res<0 (hard error) would
                //submit a new multishot against a doomed fd — the
                //dispatcher about to receive EPOLLRDHUP/EPOLLERR will
                //close() the socket and the re-armed SQE would either
                //fail (-EBADF) or worse, attach to a recycled fd
                //belonging to a future client. ENOBUFS is the one
                //negative case that DOES want a re-arm (the fd is
                //still live; the kernel just ran out of buffers).
                const bool sqe_terminated=
                    (cqe_flags & IORING_CQE_F_MORE)==0;
                const bool rearm_eligible=
                    sqe_terminated && obj!=nullptr
                    && (res>0 || res==-ENOBUFS);
                if(rearm_eligible)
                {
                    //Re-arm via the BaseClassSwitch fd accessor.
                    //EventLoopClient (and every cluster client built on
                    //it) returns the live socket fd; a closed socket
                    //returns -1 and we skip silently. Persistent failures
                    //are rate-limited so a wedged buffer ring still
                    //surfaces in the log.
                    const int rearm_fd=obj->recvMultishotFd();
                    if(rearm_fd>=0)
                    {
                        if(!armRecvMultishot(rearm_fd,obj))
                        {
                            if((g_uring->rearm_fail_count++ & 0xFF)==0)
                                std::cerr << "io_uring recv_multishot: "
                                             "armRecvMultishot re-arm "
                                             "failed for fd "<<rearm_fd
                                          <<" (x"
                                          <<g_uring->rearm_fail_count
                                          <<")"<<std::endl;
                        }
                    }
                }
            }
            else if(URING_IS_SEND_CHAIN(udata))
            {
                //Phase 3: final CQE of a datapack-send chain. All
                //intermediate CQEs (header send, per-file metadata
                //send, splice file→pipe, splice pipe→sock, close)
                //had user_data=0 and were silently dropped above.
                //We only see this one. cqe->res reflects only the
                //LAST SQE (close); a partial chain failure shows
                //up as -ECANCELED on intermediate CQEs (dropped)
                //AND -ECANCELED on this final close CQE (since
                //the cancellation propagates down the link chain).
                PendingSendOp *op=
                    static_cast<PendingSendOp *>(URING_UNTAG(udata));
                const bool success=(res>=0);
                //Close any fds that the chain didn't manage to close
                //itself (e.g. chain failed before reaching the close
                //SQEs). Best-effort, idempotent.
                size_t fi=0;
                while(fi<op->file_fds.size())
                {
                    if(op->file_fds[fi]>=0)
                    {
                        ::close(op->file_fds[fi]);
                        op->file_fds[fi]=-1;
                    }
                    fi++;
                }
                if(op->client!=nullptr)
                    op->client->onAsyncSendChainComplete(success);
                delete op;
            }
            else
            {
                void *ptr=reinterpret_cast<void *>(udata);
                if(res<0)
                {
                    //Other negative results (e.g. POLLHUP-only completions
                    //the kernel surfaced as a non-fatal error). Surface as
                    //EPOLLERR so the dispatcher cleans up the live fd.
                    events[produced].events=EPOLLERR;
                    events[produced].data.ptr=ptr;
                    ++produced;
                }
                else
                {
                    const uint32_t mask=static_cast<uint32_t>(res);
                    events[produced].events=pollMaskToEpoll(mask);
                    events[produced].data.ptr=ptr;
                    ++produced;
                }
            }
        }
        io_uring_cqe_seen(&g_uring->ring,cqe);
        //Drain any other already-completed CQEs.
        if(io_uring_peek_cqe(&g_uring->ring,&cqe)<0 || cqe==nullptr)
            break;
    }
    #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
    _event_rate_account(produced);
    #endif
    return produced;
#else
    const int produced=epoll_wait(efd, events, maxevents, -1);
    #ifdef CATCHCHALLENGER_TESTING_LIMIT_EVENT_RATE
    _event_rate_account(produced);
    #endif
    return produced;
#endif
}

int EventLoop::ctl(int __op, int __fd,epoll_event *__event)
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(efd==-1)
    {
        std::cerr << "EventLoop::ctl failed, efd==-1, call EventLoop::init() before" << std::endl;
        abort();
    }
    #endif
#if defined(CATCHCHALLENGER_SELECT)
    if(g_select==nullptr)
        return -1;
    if(__op==EPOLL_CTL_ADD || __op==EPOLL_CTL_MOD)
    {
        g_select->events[__fd]=__event->events;
        g_select->ptrs[__fd]=__event->data.ptr;
        //Re-arm the EPOLLOUT edge: a freshly added/modified interest set should
        //report writability once on the next wait() even if the fd was writable
        //before (matches epoll's EPOLLET behaviour on re-registration).
        g_select->writableReported[__fd]=false;
        return 0;
    }
    else if(__op==EPOLL_CTL_DEL)
    {
        g_select->events.erase(__fd);
        g_select->ptrs.erase(__fd);
        g_select->writableReported.erase(__fd);
        return 0;
    }
    return -1;
#elif defined(CATCHCHALLENGER_POLL)
    if(g_poll==nullptr)
        return -1;
    if(__op==EPOLL_CTL_ADD)
    {
        pollfd p;
        p.fd=__fd;
        p.events=unixToPoll(__event->events);
        p.revents=0;
        g_poll->idx[__fd]=g_poll->pfds.size();
        g_poll->pfds.push_back(p);
        g_poll->ptrs.push_back(__event->data.ptr);
        return 0;
    }
    else if(__op==EPOLL_CTL_MOD)
    {
        const std::unordered_map<int,size_t>::iterator it=g_poll->idx.find(__fd);
        if(it==g_poll->idx.end())
            return -1;
        g_poll->pfds[it->second].events=unixToPoll(__event->events);
        g_poll->ptrs[it->second]=__event->data.ptr;
        return 0;
    }
    else if(__op==EPOLL_CTL_DEL)
    {
        const std::unordered_map<int,size_t>::iterator it=g_poll->idx.find(__fd);
        if(it==g_poll->idx.end())
            return 0;
        const size_t pos=it->second;
        const size_t last=g_poll->pfds.size()-1;
        if(pos!=last)
        {
            g_poll->pfds[pos]=g_poll->pfds[last];
            g_poll->ptrs[pos]=g_poll->ptrs[last];
            g_poll->idx[g_poll->pfds[pos].fd]=pos;
        }
        g_poll->pfds.pop_back();
        g_poll->ptrs.pop_back();
        g_poll->idx.erase(it);
        return 0;
    }
    return -1;
#elif defined(CATCHCHALLENGER_IO_URING)
    if(g_uring==nullptr)
        return -1;
    if(__op==EPOLL_CTL_ADD || __op==EPOLL_CTL_MOD)
    {
        if(__op==EPOLL_CTL_MOD)
        {
            //Remove previous poll first.
            io_uring_sqe *sqe=io_uring_get_sqe(&g_uring->ring);
            if(sqe==nullptr)
                return -1;
            io_uring_prep_poll_remove(sqe,reinterpret_cast<__u64>(g_uring->ptrs[__fd]));
            io_uring_sqe_set_data(sqe,nullptr);
        }
        g_uring->ptrs[__fd]=__event->data.ptr;
        io_uring_sqe *sqe=io_uring_get_sqe(&g_uring->ring);
        if(sqe==nullptr)
            return -1;
        io_uring_prep_poll_multishot(sqe,__fd,unixMaskToPoll(__event->events));
        io_uring_sqe_set_data(sqe,__event->data.ptr);
        io_uring_submit(&g_uring->ring);
        return 0;
    }
    else if(__op==EPOLL_CTL_DEL)
    {
        const std::unordered_map<int,void *>::iterator it=g_uring->ptrs.find(__fd);
        if(it==g_uring->ptrs.end())
            return 0;
        io_uring_sqe *sqe=io_uring_get_sqe(&g_uring->ring);
        if(sqe!=nullptr)
        {
            io_uring_prep_poll_remove(sqe,reinterpret_cast<__u64>(it->second));
            io_uring_sqe_set_data(sqe,nullptr);
            io_uring_submit(&g_uring->ring);
        }
        g_uring->ptrs.erase(it);
        return 0;
    }
    return -1;
#else
    return epoll_ctl(efd, __op, __fd, __event);
#endif
}

#ifdef CATCHCHALLENGER_IO_URING
bool EventLoop::multishotEnabled() const
{
    if(g_uring==nullptr)
        return false;
    return g_uring->multishot_enabled;
}

bool EventLoop::armRecvMultishot(int fd,void *user_data)
{
    if(g_uring==nullptr || !g_uring->multishot_enabled)
        return false;
    io_uring_sqe *sqe=io_uring_get_sqe(&g_uring->ring);
    if(sqe==nullptr)
        return false;
    io_uring_prep_recv_multishot(sqe,fd,nullptr,0,0);
    sqe->buf_group=0;
    sqe->flags |= IOSQE_BUFFER_SELECT;
    io_uring_sqe_set_data64(sqe,URING_TAG_RECV(user_data));
    if(io_uring_submit(&g_uring->ring)<0)
        return false;
    return true;
}

bool EventLoop::uringEnabled() const
{
    return g_uring!=nullptr;
}

bool EventLoop::submitDatapackChain(int sock_fd,
                                    int pipe_r,int pipe_w,
                                    const char *header_bytes,size_t header_len,
                                    const std::vector<std::string> &per_file_meta,
                                    const std::vector<int> &file_fds,
                                    const std::vector<std::vector<size_t> > &per_file_chunks,
                                    BaseClassSwitch *client)
{
    if(g_uring==nullptr)
        return false;
    if(per_file_meta.size()!=file_fds.size()
       || per_file_meta.size()!=per_file_chunks.size())
    {
        std::cerr << "submitDatapackChain: vector size mismatch" << std::endl;
        return false;
    }
    //Pre-count required SQEs so we can reject early if the SQ can't fit.
    //Per file: 1 send (metadata) + 2 splice per chunk + 1 close.
    //Plus 1 leading header send.
    size_t needed=(header_len>0?1:0);
    size_t fi=0;
    while(fi<per_file_meta.size())
    {
        needed+=1;//metadata send
        needed+=2*per_file_chunks[fi].size();//splice in + splice out
        needed+=1;//close
        fi++;
    }
    if(needed==0)
        return false;
    if(io_uring_sq_space_left(&g_uring->ring)<needed)
        return false;
    //Heap-own all bytes referenced by SQEs so they outlive submission.
    PendingSendOp *op=new PendingSendOp();
    op->client=client;
    op->file_fds=file_fds;
    op->header_buf.assign(header_bytes,header_len);
    op->meta=per_file_meta;
    //Build chain. Every SQE except the last gets IOSQE_IO_LINK; every
    //SQE except the last gets user_data=0 so the CQE drain drops it.
    //The last SQE carries URING_TAG_SEND_CHAIN(op) so we get exactly
    //one final completion to free state.
    io_uring_sqe *sqe;
    //Header send.
    if(header_len>0)
    {
        sqe=io_uring_get_sqe(&g_uring->ring);
        if(sqe==nullptr)
        {
            delete op;
            return false;
        }
        io_uring_prep_send(sqe,sock_fd,op->header_buf.data(),
                           op->header_buf.size(),MSG_NOSIGNAL);
        sqe->flags |= IOSQE_IO_LINK;
        io_uring_sqe_set_data64(sqe,0);
    }
    fi=0;
    while(fi<per_file_meta.size())
    {
        const bool is_last_file=(fi+1==per_file_meta.size());
        //Per-file metadata send.
        sqe=io_uring_get_sqe(&g_uring->ring);
        if(sqe==nullptr)
        {
            //SQ exhaustion mid-build. We've already submitted nothing
            //(io_uring_submit not yet called), but the partially-built
            //chain occupies SQ slots. Drop them by submitting now —
            //they'll race with previously-arming-only SQEs but the
            //link chain breaks here so consequences are bounded. KISS.
            delete op;
            return false;
        }
        io_uring_prep_send(sqe,sock_fd,op->meta[fi].data(),
                           op->meta[fi].size(),MSG_NOSIGNAL);
        sqe->flags |= IOSQE_IO_LINK;
        io_uring_sqe_set_data64(sqe,0);
        //Per-chunk splice pair.
        size_t ci=0;
        while(ci<per_file_chunks[fi].size())
        {
            const size_t chunk=per_file_chunks[fi][ci];
            //file -> pipe
            sqe=io_uring_get_sqe(&g_uring->ring);
            if(sqe==nullptr)
            {
                delete op;
                return false;
            }
            io_uring_prep_splice(sqe,file_fds[fi],-1,pipe_w,-1,
                                 chunk,SPLICE_F_MOVE);
            sqe->flags |= IOSQE_IO_LINK;
            io_uring_sqe_set_data64(sqe,0);
            //pipe -> sock
            sqe=io_uring_get_sqe(&g_uring->ring);
            if(sqe==nullptr)
            {
                delete op;
                return false;
            }
            io_uring_prep_splice(sqe,pipe_r,-1,sock_fd,-1,
                                 chunk,SPLICE_F_MOVE);
            sqe->flags |= IOSQE_IO_LINK;
            io_uring_sqe_set_data64(sqe,0);
            ci++;
        }
        //Close the file fd. Last SQE in chain (only if last file): final
        //CQE that surfaces, tagged with op pointer.
        sqe=io_uring_get_sqe(&g_uring->ring);
        if(sqe==nullptr)
        {
            delete op;
            return false;
        }
        io_uring_prep_close(sqe,file_fds[fi]);
        if(is_last_file)
        {
            io_uring_sqe_set_data64(sqe,URING_TAG_SEND_CHAIN(op));
            //Last SQE: do NOT set IOSQE_IO_LINK (chain ends here).
        }
        else
        {
            sqe->flags |= IOSQE_IO_LINK;
            io_uring_sqe_set_data64(sqe,0);
        }
        fi++;
    }
    if(io_uring_submit(&g_uring->ring)<0)
    {
        std::cerr << "submitDatapackChain: io_uring_submit failed" << std::endl;
        delete op;
        return false;
    }
    return true;
}
#endif

ssize_t EventLoop::sendFile(int sock_fd,int in_fd,off_t *offset,size_t len)
{
    //All three backends use sendfile(2): it's already zero-copy in the
    //kernel sense — bytes move directly from the page cache to the
    //socket's send queue, no userspace bounce buffer.
    //
    //For the CATCHCHALLENGER_IO_URING backend we deliberately do NOT
    //submit splice/sendfile SQEs here: doing so would interleave the
    //transfer's completion CQEs with the poll-multishot CQEs the main
    //loop drains, and the existing datapack send path is synchronous —
    //the caller expects a syscall-style {bytes-sent, errno} result.
    //Making datapack send fully async over io_uring requires turning
    //ClientHeavyLoadMirror::sendFile into a state machine driven by
    //completion callbacks (separate task; see comment in
    //ClientHeavyLoadMirror.cpp::sendFile).
#ifdef _WIN32
    //Windows has no sendfile(2) in POSIX form. Bounce through a small
    //userspace buffer: read up to 4 KiB from the file at *offset, ship
    //it to the socket via send(), advance *offset by the bytes the
    //socket actually accepted. Mirrors the kernel sendfile(2) semantics
    //the rest of the code expects (return total bytes sent or -1).
    char buf[4096];
    size_t total=0;
    while(total<len)
    {
        const size_t want=(len-total<sizeof(buf))?(len-total):sizeof(buf);
        if(::_lseeki64(in_fd,static_cast<__int64>(*offset),SEEK_SET)==(__int64)-1)
            return -1;
        const int r=::_read(in_fd,buf,static_cast<unsigned int>(want));
        if(r<0)
            return -1;
        if(r==0)
            return static_cast<ssize_t>(total);
        const int s=::send(static_cast<SOCKET>(sock_fd),buf,r,0);
        if(s==SOCKET_ERROR)
        {
            if(total>0)
                return static_cast<ssize_t>(total);
            return -1;
        }
        *offset+=s;
        total+=static_cast<size_t>(s);
        if(s<r)
            return static_cast<ssize_t>(total);
    }
    return static_cast<ssize_t>(total);
#elif defined(__DJGPP__) || defined(CC_TARGET_ESP32)
    //MS-DOS (Watt-32) and ESP32 (lwIP) have no sendfile(2): same userspace
    //bounce as Windows, POSIX lseek/read for the file + send() for the socket.
    char buf[4096];
    size_t total=0;
    while(total<len)
    {
        const size_t want=(len-total<sizeof(buf))?(len-total):sizeof(buf);
        if(::lseek(in_fd,*offset,SEEK_SET)==(off_t)-1)
            return -1;
        const ssize_t r=::read(in_fd,buf,want);
        if(r<0)
            return -1;
        if(r==0)
            return static_cast<ssize_t>(total);
        const int s=::send(sock_fd,buf,static_cast<int>(r),0);
        if(s<0)
        {
            if(total>0)
                return static_cast<ssize_t>(total);
            return -1;
        }
        *offset+=s;
        total+=static_cast<size_t>(s);
        if(s<static_cast<int>(r))
            return static_cast<ssize_t>(total);
    }
    return static_cast<ssize_t>(total);
#else
    return ::sendfile(sock_fd,in_fd,offset,len);
#endif
}
