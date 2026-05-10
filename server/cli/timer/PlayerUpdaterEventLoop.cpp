#include "PlayerUpdaterEventLoop.hpp"

PlayerUpdaterEventLoop::PlayerUpdaterEventLoop()
{
    setInterval(1000);
}

void PlayerUpdaterEventLoop::setInterval(int ms)
{
    EventLoopTimer::setInterval(ms);
}

void PlayerUpdaterEventLoop::exec()
{
    PlayerUpdaterBase::exec();
}
