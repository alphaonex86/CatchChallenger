#include "Epoll.hpp"
#include "../../general/base/GeneralVariable.hpp"

#include <iostream>
#include <sys/sendfile.h>
#include <unistd.h>

#if defined(CATCHCHALLENGER_POLL)
    #include <poll.h>
    #include <vector>
    #include <unordered_map>
#elif defined(CATCHCHALLENGER_IO_URING)
    #include <liburing.h>
    #include <vector>
    #include <unordered_map>
    #include <fcntl.h>
#endif

Epoll Epoll::epoll;

#if defined(CATCHCHALLENGER_POLL)
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

short epollToPoll(uint32_t e)
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
#include <poll.h>
namespace {
struct UringState
{
    io_uring ring;
    std::unordered_map<int,void *> ptrs;//fd -> user data
};
static UringState *g_uring=nullptr;

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
short epollMaskToPoll(uint32_t e)
{
    short r=0;
    if(e & EPOLLIN)    r|=POLLIN;
    if(e & EPOLLOUT)   r|=POLLOUT;
    if(e & EPOLLRDHUP) r|=POLLRDHUP;
    return r;
}
}//namespace
#endif

Epoll::Epoll()
{
    efd=-1;
}

Epoll::~Epoll()
{
#if defined(CATCHCHALLENGER_POLL)
    delete g_poll;
    g_poll=nullptr;
#elif defined(CATCHCHALLENGER_IO_URING)
    if(g_uring!=nullptr)
    {
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

bool Epoll::init()
{
#if defined(CATCHCHALLENGER_POLL)
    g_poll=new PollState();
    efd=0;//unused but flips the "initialised" check
    return true;
#elif defined(CATCHCHALLENGER_IO_URING)
    g_uring=new UringState();
    //Queue depth 4096 covers an MMORPG with hundreds of concurrent fds
    //(one poll-multishot SQE per fd plus splice/sendfile chains).
    if(io_uring_queue_init(4096,&g_uring->ring,0)<0)
    {
        std::cerr << "io_uring_queue_init failed" << std::endl;
        delete g_uring;
        g_uring=nullptr;
        return false;
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

int Epoll::wait(epoll_event *events,const int &maxevents)
{
#if defined(CATCHCHALLENGER_POLL)
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
        if(cqe->res<0)
        {
            //Cancelled poll, removed fd, errno-style negative.
            //Surface as EPOLLERR so the dispatcher cleans up.
            void *ptr=reinterpret_cast<void *>(cqe->user_data);
            events[produced].events=EPOLLERR;
            events[produced].data.ptr=ptr;
            ++produced;
        }
        else
        {
            const uint32_t mask=static_cast<uint32_t>(cqe->res);
            void *ptr=reinterpret_cast<void *>(cqe->user_data);
            events[produced].events=pollMaskToEpoll(mask);
            events[produced].data.ptr=ptr;
            ++produced;
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

int Epoll::ctl(int __op, int __fd,epoll_event *__event)
{
    #ifdef CATCHCHALLENGER_HARDENED
    if(efd==-1)
    {
        std::cerr << "Epoll::ctl failed, efd==-1, call Epoll::init() before" << std::endl;
        abort();
    }
    #endif
#if defined(CATCHCHALLENGER_POLL)
    if(g_poll==nullptr)
        return -1;
    if(__op==EPOLL_CTL_ADD)
    {
        pollfd p;
        p.fd=__fd;
        p.events=epollToPoll(__event->events);
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
        g_poll->pfds[it->second].events=epollToPoll(__event->events);
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
        io_uring_prep_poll_multishot(sqe,__fd,epollMaskToPoll(__event->events));
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

ssize_t Epoll::sendFile(int sock_fd,int in_fd,off_t *offset,size_t len)
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
    return ::sendfile(sock_fd,in_fd,offset,len);
}
