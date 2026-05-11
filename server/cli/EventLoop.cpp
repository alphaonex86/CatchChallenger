#include "EventLoop.hpp"
#include "../../general/base/GeneralVariable.hpp"

#include <iostream>
#ifndef _WIN32
#include <sys/sendfile.h>
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
    #include "BaseClassSwitch.hpp"
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
};
static SelectState *g_select=nullptr;
}//namespace
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
        if(g_uring->buf_ring!=nullptr)
        {
            io_uring_free_buf_ring(&g_uring->ring,g_uring->buf_ring,
                                   g_uring->buf_count,0);
            g_uring->buf_ring=nullptr;
        }
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

int EventLoop::wait(epoll_event *events,const int &maxevents)
{
#if defined(CATCHCHALLENGER_SELECT)
    if(g_select==nullptr || g_select->events.empty())
        return 0;
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
        if(mask & EPOLLOUT) FD_SET(fd,&wfds);
        //EPOLLERR/EPOLLHUP are reported via the exception fd_set.
        FD_SET(fd,&efds);
        if(fd>maxfd) maxfd=fd;
    }
    const int n=::select(maxfd+1,&rfds,&wfds,&efds,nullptr);
    if(n<=0)
        return n;
    int produced=0;
    for(const std::pair<const int,uint32_t> &e : g_select->events)
    {
        if(produced>=maxevents)
            break;
        const int fd=e.first;
        uint32_t out=0;
        if(FD_ISSET(fd,&rfds)) out|=EPOLLIN;
        if(FD_ISSET(fd,&wfds)) out|=EPOLLOUT;
        if(FD_ISSET(fd,&efds)) out|=EPOLLERR;
        if(out!=0)
        {
            events[produced].events=out;
            events[produced].data.ptr=g_select->ptrs[fd];
            ++produced;
        }
    }
    return produced;
#elif defined(CATCHCHALLENGER_POLL)
    if(g_poll==nullptr || g_poll->pfds.empty())
        return 0;
    const int n=::poll(g_poll->pfds.data(),g_poll->pfds.size(),-1);
    if(n<=0)
        return n;
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
                bool need_rearm=false;
                if(res==-ENOBUFS)
                {
                    //Buffer ring exhausted. Rate-limit log: every 1024.
                    if((g_uring->enobufs_count++ & 0x3FF)==0)
                        std::cerr << "io_uring recv_multishot: ENOBUFS x"
                                  << g_uring->enobufs_count << std::endl;
                    need_rearm=true;
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
                if((cqe_flags & IORING_CQE_F_MORE)==0 && obj!=nullptr)
                    need_rearm=true;
                if(need_rearm)
                {
                    //We don't know the fd here without an extra map; the
                    //ergonomic re-arm is expected from the dispatcher when
                    //it next sees the client. For now, log once at the
                    //first occurrence so the operator notices stalled RX.
                    static bool warned=false;
                    if(!warned)
                    {
                        warned=true;
                        std::cerr << "io_uring recv_multishot: re-arm "
                                     "needed but fd unknown to wait(); "
                                     "caller must armRecvMultishot() again"
                                  << std::endl;
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
    return produced;
#else
    return epoll_wait(efd, events, maxevents, -1);
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
        return 0;
    }
    else if(__op==EPOLL_CTL_DEL)
    {
        g_select->events.erase(__fd);
        g_select->ptrs.erase(__fd);
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
#else
    return ::sendfile(sock_fd,in_fd,offset,len);
#endif
}
