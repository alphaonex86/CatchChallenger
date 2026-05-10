#include "PlayerUpdaterEpoll.hpp"

PlayerUpdaterEpoll::PlayerUpdaterEpoll()
{
    setInterval(1000);
}

void PlayerUpdaterEpoll::setInterval(int ms)
{
    EpollTimer::setInterval(ms);
}

void PlayerUpdaterEpoll::exec()
{
    PlayerUpdaterBase::exec();
}
