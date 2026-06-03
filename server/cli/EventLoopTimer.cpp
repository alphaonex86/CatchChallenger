#include "EventLoopTimer.hpp"
#include "EventLoop.hpp"
#include "win32_compat.hpp"

#include <iostream>
#if !defined(_WIN32) && !defined(__DJGPP__)
#include <sys/timerfd.h>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include <time.h>

char buff_temp[sizeof(uint64_t)];

#ifdef __DJGPP__
//Stored in tfd while a DOS timer is armed: it lives in the EventLoop's timer
//registry, not behind a real fd, so any non-(-1) value works as the
//"active"/"already armed" marker.
static const int CC_DOS_TIMER_ARMED=0x7fffffff;
#endif

#ifdef _WIN32
//-----------------------------------------------------------------------
// Windows port of the timerfd backend: a Win32 waitable timer fired by a
// thread pool that, on each tick, writes one byte into a self-pipe made
// of two AF_INET loopback sockets. The read end is what we register into
// the EventLoop (select(2) sees it become readable); validateTheTimer()
// drains that byte to mimic the timerfd read-the-expiration-count
// behaviour Linux uses.
//-----------------------------------------------------------------------

#include <windows.h>
#include <threadpoolapiset.h>

namespace {

struct CcTimerCtx
{
    SOCKET write_sock;
    PTP_TIMER tp_timer;
};

//Create a (read,write) AF_INET socket pair on 127.0.0.1.
//Mirrors the trick used to emulate socketpair() on Windows.
bool cc_make_socket_pair(SOCKET *out_read, SOCKET *out_write)
{
    SOCKET listener=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(listener==INVALID_SOCKET)
        return false;
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    addr.sin_port=0;
    if(::bind(listener,reinterpret_cast<sockaddr *>(&addr),sizeof(addr))==SOCKET_ERROR)
    {
        ::closesocket(listener);
        return false;
    }
    int alen=sizeof(addr);
    if(::getsockname(listener,reinterpret_cast<sockaddr *>(&addr),&alen)==SOCKET_ERROR)
    {
        ::closesocket(listener);
        return false;
    }
    if(::listen(listener,1)==SOCKET_ERROR)
    {
        ::closesocket(listener);
        return false;
    }
    SOCKET client=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(client==INVALID_SOCKET)
    {
        ::closesocket(listener);
        return false;
    }
    if(::connect(client,reinterpret_cast<sockaddr *>(&addr),sizeof(addr))==SOCKET_ERROR)
    {
        ::closesocket(client);
        ::closesocket(listener);
        return false;
    }
    SOCKET server=::accept(listener,NULL,NULL);
    if(server==INVALID_SOCKET)
    {
        ::closesocket(client);
        ::closesocket(listener);
        return false;
    }
    ::closesocket(listener);
    *out_read=server;
    *out_write=client;
    return true;
}

VOID CALLBACK cc_timer_callback(PTP_CALLBACK_INSTANCE,PVOID context,PTP_TIMER)
{
    CcTimerCtx *ctx=static_cast<CcTimerCtx *>(context);
    if(ctx==NULL)
        return;
    char one='!';
    ::send(ctx->write_sock,&one,1,0);
}

}//namespace
#endif // _WIN32

EventLoopTimer::EventLoopTimer() :
    tfd(-1),
    msec(0),
    offset(0),
    singleShot(false)
#ifdef _WIN32
    ,timer_ctx(NULL)
#endif
{
}

EventLoopTimer::~EventLoopTimer()
{
    stop();
    //mostly bad usage for my own upper class
    //abort();
}

BaseClassSwitch::EventLoopObjectType EventLoopTimer::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Timer;
}

bool EventLoopTimer::start(const unsigned int &msec, unsigned int offset)
{
    if(tfd!=-1)
        return false;
    if(msec<1)
        return false;
    if(offset==0)
        offset=msec;
#ifdef _WIN32
    SOCKET rsock=INVALID_SOCKET, wsock=INVALID_SOCKET;
    if(!cc_make_socket_pair(&rsock,&wsock))
    {
        std::cerr << "Timer creation error (socket pair)" << std::endl;
        return false;
    }
    CcTimerCtx *ctx=new CcTimerCtx();
    ctx->write_sock=wsock;
    ctx->tp_timer=::CreateThreadpoolTimer(cc_timer_callback,ctx,NULL);
    if(ctx->tp_timer==NULL)
    {
        std::cerr << "CreateThreadpoolTimer failed" << std::endl;
        ::closesocket(wsock);
        ::closesocket(rsock);
        delete ctx;
        return false;
    }
    //Negative FILETIME = relative due time, in 100ns units.
    FILETIME ft;
    LARGE_INTEGER li;
    li.QuadPart=-static_cast<LONGLONG>(offset)*10000LL;
    ft.dwLowDateTime=li.LowPart;
    ft.dwHighDateTime=static_cast<DWORD>(li.HighPart);
    const DWORD period=singleShot?0:msec;
    ::SetThreadpoolTimer(ctx->tp_timer,&ft,period,0);
    tfd=static_cast<int>(rsock);
    timer_ctx=ctx;
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.data.ptr = this;
    event.events = EPOLLIN;
    if(EventLoop::loop.ctl(EPOLL_CTL_ADD,tfd,&event) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }
    return true;
#elif defined(__DJGPP__)
    //-------------------------------------------------------------------
    // MS-DOS (DJGPP): no timerfd and no threads. Register the interval with
    // the EventLoop; wait() sleeps on select(2)'s timeout until it is due and
    // returns a synthetic event that the main loop dispatches to exec().
    // tfd holds a non-fd sentinel so active() and the already-armed guard work.
    //-------------------------------------------------------------------
    EventLoop::loop.dosTimerArm(this,msec,offset,singleShot);
    tfd=CC_DOS_TIMER_ARMED;
    return true;
#else
    if((tfd=::timerfd_create(CLOCK_REALTIME,TFD_NONBLOCK)) < 0)
    {
        std::cerr << "Timer creation error" << std::endl;
        return false;
    }

    timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) == -1)
    {
        std::cerr << "clock_gettime error" << std::endl;
        return false;
    }
    itimerspec new_value;
    if(singleShot)
    {
        new_value.it_value.tv_sec = now.tv_sec + msec/1000;
        new_value.it_value.tv_nsec = now.tv_nsec + (msec%1000)*1000000;
        if(new_value.it_value.tv_nsec>999999999)
        {
            new_value.it_value.tv_nsec-=1000000000;
            new_value.it_value.tv_sec++;
        }
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;
    }
    else
    {
        new_value.it_value.tv_sec = now.tv_sec + offset/1000;
        new_value.it_value.tv_nsec = now.tv_nsec + (offset%1000)*1000000;
        if(new_value.it_value.tv_nsec>999999999)
        {
            new_value.it_value.tv_nsec-=1000000000;
            new_value.it_value.tv_sec++;
        }
        new_value.it_interval.tv_sec = msec/1000;
        new_value.it_interval.tv_nsec = (msec%1000)*1000000;
        if(new_value.it_interval.tv_nsec>999999999)
        {
            new_value.it_interval.tv_nsec-=1000000000;
            new_value.it_interval.tv_sec++;
        }
    }

    const int &result=::timerfd_settime(tfd, TFD_TIMER_ABSTIME, &new_value, NULL);
    if(result<0)
    {
        //settime error: 22: Invalid argument
        std::cerr << "settime error: " << errno << ": " << strerror(errno) << std::endl;
        return false;
    }
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.data.ptr = this;
    event.events = EPOLLIN;
    if(EventLoop::loop.ctl(EPOLL_CTL_ADD,tfd,&event) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }
    return true;
#endif
}

bool EventLoopTimer::start()
{
    return start(msec,offset);
}

bool EventLoopTimer::stop()
{
    if(tfd==-1)
        return false;
#ifdef __DJGPP__
    //DOS: the timer is in the EventLoop registry, not an fd — just unregister.
    EventLoop::loop.dosTimerDisarm(this);
    tfd=-1;
    return true;
#else
    if(EventLoop::loop.ctl(EPOLL_CTL_DEL,tfd,NULL) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        return false;
    }
#ifdef _WIN32
    if(timer_ctx!=NULL)
    {
        CcTimerCtx *ctx=static_cast<CcTimerCtx *>(timer_ctx);
        ::SetThreadpoolTimer(ctx->tp_timer,NULL,0,0);
        ::WaitForThreadpoolTimerCallbacks(ctx->tp_timer,TRUE);
        ::CloseThreadpoolTimer(ctx->tp_timer);
        ::closesocket(ctx->write_sock);
        delete ctx;
        timer_ctx=NULL;
    }
    ::closesocket(static_cast<SOCKET>(tfd));
#else
    ::close(tfd);
#endif
    tfd=-1;
    return true;
#endif
}

bool EventLoopTimer::active()
{
    return tfd!=-1;
}

void EventLoopTimer::setInterval(const unsigned int &msec,const unsigned int &offset)
{
    this->msec=msec;
    if(offset==0)
        this->offset=msec;
    else
        this->offset=offset;
    if(tfd!=-1)
    {
        stop();
        start();
    }
}

void EventLoopTimer::setSingleShot(const bool &singleShot)
{
    this->singleShot=singleShot;
}

void EventLoopTimer::validateTheTimer()
{
#ifdef _WIN32
    char buf[16];
    //Drain whatever bytes the callback enqueued; we don't care about count.
    ::recv(static_cast<SOCKET>(tfd),buf,sizeof(buf),0);
#elif defined(__DJGPP__)
    //Nothing to drain: EventLoop::wait() already rescheduled this timer when
    //it synthesized the tick event.
#else
    if(::read(tfd, buff_temp, sizeof(uint64_t))!=sizeof(uint64_t))
    {}
#endif
}
