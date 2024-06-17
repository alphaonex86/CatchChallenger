#include "TimerMove.hpp"
#include "Bot.hpp"

TimerMove::TimerMove()
{
}

void TimerMove::exec()
{
    unsigned int index=0;
    while(index<Bot::list.size())
    {
        Bot::list.at(index)->generateNewMove();
        index++;
    }
}
