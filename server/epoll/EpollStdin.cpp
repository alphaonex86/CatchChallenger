#include "EpollStdin.h"
#include "Epoll.h"
#include <iostream>
#include <cstring>

char buff_input[64];

EpollStdin::EpollStdin()
{
    epoll_event event;
    event.data.ptr = this;
    event.events = EPOLLIN;
    if(Epoll::epoll.ctl(EPOLL_CTL_ADD,_IO_feof(stdin),&event) < 0)
    {
        std::cerr << "epoll_ctl error" << std::endl;
        abort();
    }
}

EpollStdin::~EpollStdin()
{
}

BaseClassSwitch::EpollObjectType EpollStdin::getType() const
{
    return BaseClassSwitch::EpollObjectType::Stdin;
}

void EpollStdin::read()
{
    const char * const r=fgets(buff_input,sizeof(buff_input),stdin);
    if(r!=NULL)
        input(buff_input,strlen(r));
}
