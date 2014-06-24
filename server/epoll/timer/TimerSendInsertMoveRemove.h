#ifndef TIMERSENDINSERTMOVEREMOVE_H
#define TIMERSENDINSERTMOVEREMOVE_H

#include "../EpollTimer.h"

class TimerSendInsertMoveRemove : public EpollTimer
{
public:
    TimerSendInsertMoveRemove();
    void exec();
};

#endif // TIMERSENDINSERTMOVEREMOVE_H
