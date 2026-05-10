#ifndef TIMERSENDINSERTMOVEREMOVE_H
#define TIMERSENDINSERTMOVEREMOVE_H

#include "../EpollTimer.hpp"

class TimerSendInsertMoveRemove : public EpollTimer
{
public:
    TimerSendInsertMoveRemove();
    void exec();
};

#endif // TIMERSENDINSERTMOVEREMOVE_H
