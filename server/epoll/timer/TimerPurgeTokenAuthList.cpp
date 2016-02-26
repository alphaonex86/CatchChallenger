#include "TimerPurgeTokenAuthList.h"
#include "../Epoll.h"

#include "../../base/Client.h"

#include <iostream>

TimerPurgeTokenAuthList::TimerPurgeTokenAuthList()
{
}

void TimerPurgeTokenAuthList::exec()
{
    #ifdef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        CatchChallenger::Client::purgeTokenAuthList();
    #else
        #error CATCHCHALLENGER_CLASS_ONLYGAMESERVER not defined
    #endif
}
