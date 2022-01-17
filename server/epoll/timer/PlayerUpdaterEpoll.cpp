#include "PlayerUpdaterEpoll.hpp"

void PlayerUpdaterEpoll::setInterval(int ms)
{
    EpollTimer::setInterval(ms);
}

void PlayerUpdaterEpoll::exec()
{
    PlayerUpdaterBase::exec();
}
