#ifndef _WIN32
#include "EventLoopStdin.h"
#include "EventLoop.h"
#include <iostream>
#include <cstring>

char buff_input[64];

EventLoopStdin::EventLoopStdin()
{
    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN;
    if(EventLoop::loop.ctl(EPOLL_CTL_ADD,_IO_feof(stdin),&event) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        abort();
    }
}

EventLoopStdin::~EventLoopStdin()
{
}

BaseClassSwitch::EventLoopObjectType EventLoopStdin::getType() const
{
    return BaseClassSwitch::EventLoopObjectType::Stdin;
}

void EventLoopStdin::read()
{
    const char * const r=fgets(buff_input,sizeof(buff_input),stdin);
    if(r!=NULL)
    {
        std::cout << "EventLoopStdin::read() ok" << std::endl;
        input(buff_input,strlen(r));
    }
    else
        std::cerr << "EventLoopStdin::read() error" << std::endl;
}
#endif // !_WIN32
