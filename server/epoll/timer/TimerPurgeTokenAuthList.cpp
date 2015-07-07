#include "TimerPurgeTokenAuthList.h"
#include "../Epoll.h"

#include "../../base/Client.h"

#include <iostream>

TimerPurgeTokenAuthList::TimerPurgeTokenAuthList()
{
}

void TimerPurgeTokenAuthList::exec()
{
    CatchChallenger::Client::purgeTokenAuthList();
}
