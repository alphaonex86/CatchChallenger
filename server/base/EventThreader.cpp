#include "EventThreader.h"

#ifndef EPOLLCATCHCHALLENGERSERVER
using namespace CatchChallenger;
EventThreader::EventThreader()
{
    setObjectName("EventThreader");
    start();
    moveToThread(this);
}

EventThreader::~EventThreader()
{
    quit();
    wait();
}

void EventThreader::run()
{
    exec();
}

void EventThreader::probe_latency()
{
    emit return_latency();
}
#endif
