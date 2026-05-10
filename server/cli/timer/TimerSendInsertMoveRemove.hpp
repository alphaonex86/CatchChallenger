#ifndef TIMERSENDINSERTMOVEREMOVE_H
#define TIMERSENDINSERTMOVEREMOVE_H

#include "../EventLoopTimer.hpp"

class TimerSendInsertMoveRemove : public EventLoopTimer
{
public:
    TimerSendInsertMoveRemove();
    void exec();
};

#endif // TIMERSENDINSERTMOVEREMOVE_H
